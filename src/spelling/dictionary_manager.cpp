/*
	SPDX-FileCopyrightText: 2009-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "dictionary_manager.h"

#ifndef Q_OS_MAC
#include "dictionary_provider_hunspell.h"
#include "dictionary_provider_voikko.h"
#else
#include "dictionary_provider_nsspellchecker.h"
#endif
#include "dictionary_ref.h"
#include "smart_quotes.h"
#include "utils.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>

//-----------------------------------------------------------------------------

namespace
{

class DictionaryFallback : public AbstractDictionary
{
public:
	static AbstractDictionary** instance()
	{
		static DictionaryFallback fallback;
		static AbstractDictionary* fallback_ptr = &fallback;
		return &fallback_ptr;
	}

	bool isValid() const override
	{
		return true;
	}

	WordRef check(const QString& string, int start_at) const override
	{
		Q_UNUSED(string);
		Q_UNUSED(start_at);
		return WordRef();
	}

	QStringList suggestions(const QString& word) const override
	{
		Q_UNUSED(word);
		return QStringList();
	}

	void addToPersonal(const QString& word) override
	{
		Q_UNUSED(word);
	}

	void addToSession(const QStringList& words) override
	{
		Q_UNUSED(words);
	}

	void removeFromSession(const QStringList& words) override
	{
		Q_UNUSED(words);
	}

private:
	DictionaryFallback()
	{
	}
};

}

QString DictionaryManager::m_path;

//-----------------------------------------------------------------------------

DictionaryManager& DictionaryManager::instance()
{
	static DictionaryManager manager;
	return manager;
}

//-----------------------------------------------------------------------------

QStringList DictionaryManager::availableDictionaries() const
{
	QStringList result;
	for (AbstractDictionaryProvider* provider : m_providers) {
		result += provider->availableDictionaries();
	}
	result.sort();
	result.removeDuplicates();
	return result;
}

//-----------------------------------------------------------------------------

QString DictionaryManager::availableDictionary(const QString& language) const
{
	const QStringList languages = availableDictionaries();
	if (!languages.isEmpty() && !languages.contains(language)) {
		const int close = languages.indexOf(QRegularExpression(language.left(2) + ".*"));
		return (close != -1) ? languages.at(close) : (languages.contains("en_US") ? "en_US" : languages.constFirst());
	} else {
		return language;
	}
}

//-----------------------------------------------------------------------------

void DictionaryManager::add(const QString& word)
{
	QStringList words = personal();
	if (words.contains(SmartQuotes::revert(word))) {
		return;
	}
	words.append(word);
	setPersonal(words);
}

//-----------------------------------------------------------------------------

void DictionaryManager::addProviders()
{
#ifndef Q_OS_MAC
	bool has_hunspell = false;
	bool has_voikko = false;

	for (AbstractDictionaryProvider* provider : std::as_const(m_providers)) {
		if (dynamic_cast<DictionaryProviderHunspell*>(provider)) {
			has_hunspell = true;
		} else if (dynamic_cast<DictionaryProviderVoikko*>(provider)) {
			has_voikko = true;
		}
	}

	if (!has_hunspell) {
		addProvider(new DictionaryProviderHunspell);
	}
	if (!has_voikko) {
		addProvider(new DictionaryProviderVoikko);
	}
#else
	bool has_nsspellchecker = false;

	for (AbstractDictionaryProvider* provider : qAsConst(m_providers)) {
		if (dynamic_cast<DictionaryProviderNSSpellChecker*>(provider)) {
			has_nsspellchecker = true;
		}
	}

	if (!has_nsspellchecker) {
		addProvider(new DictionaryProviderNSSpellChecker);
	}
#endif
}

//-----------------------------------------------------------------------------

DictionaryRef DictionaryManager::requestDictionary(const QString& language)
{
	if (language.isEmpty()) {
		// Fetch shared default dictionary
		if (!m_default_dictionary) {
			m_default_dictionary = *requestDictionaryData(m_default_language);
		}
		return DictionaryRef(&m_default_dictionary);
	} else {
		// Fetch specific dictionary
		return DictionaryRef(requestDictionaryData(language));
	}
}

//-----------------------------------------------------------------------------

void DictionaryManager::setDefaultLanguage(const QString& language)
{
	if (language == m_default_language) {
		return;
	}

	m_default_language = language;
	m_default_dictionary = *requestDictionaryData(m_default_language);

	// Re-check documents
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

void DictionaryManager::setIgnoreNumbers(bool ignore)
{
	for (AbstractDictionaryProvider* provider : std::as_const(m_providers)) {
		provider->setIgnoreNumbers(ignore);
	}

	// Re-check documents
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

void DictionaryManager::setIgnoreUppercase(bool ignore)
{
	for (AbstractDictionaryProvider* provider : std::as_const(m_providers)) {
		provider->setIgnoreUppercase(ignore);
	}

	// Re-check documents
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

QString DictionaryManager::installedPath()
{
#ifndef Q_OS_MAC
	return m_path;
#else
	return QDir::homePath() + "/Library/Spelling/";
#endif
}

//-----------------------------------------------------------------------------

void DictionaryManager::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------

void DictionaryManager::setPersonal(const QStringList& words)
{
	// Check if new
	QStringList personal = SmartQuotes::revert(words);
	std::sort(personal.begin(), personal.end(), localeAwareSort);
	if (personal == m_personal) {
		return;
	}

	// Remove current personal dictionary
	for (AbstractDictionary* dictionary : std::as_const(m_dictionaries)) {
		dictionary->removeFromSession(m_personal);
	}

	// Update and store personal dictionary
	m_personal = personal;
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		for (const QString& word : std::as_const(m_personal)) {
			stream << word << "\n";
		}
	}

	// Add personal dictionary
	for (AbstractDictionary* dictionary : std::as_const(m_dictionaries)) {
		dictionary->addToSession(m_personal);
	}

	// Re-check documents
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

DictionaryManager::DictionaryManager()
{
	addProviders();

	// Load personal dictionary
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		while (!stream.atEnd()) {
			m_personal.append(stream.readLine());
		}
		std::sort(m_personal.begin(), m_personal.end(), localeAwareSort);
	}
}

//-----------------------------------------------------------------------------

DictionaryManager::~DictionaryManager()
{
	for (AbstractDictionary* dictionary : std::as_const(m_dictionaries)) {
		delete dictionary;
	}
	m_dictionaries.clear();

	qDeleteAll(m_providers);
	m_providers.clear();
}

//-----------------------------------------------------------------------------

void DictionaryManager::addProvider(AbstractDictionaryProvider* provider)
{
	if (provider->isValid()) {
		m_providers.append(provider);
	} else {
		delete provider;
		provider = nullptr;
	}
}

//-----------------------------------------------------------------------------

AbstractDictionary** DictionaryManager::requestDictionaryData(const QString& language)
{
	if (!m_dictionaries.contains(language)) {
		AbstractDictionary* dictionary = nullptr;
		for (AbstractDictionaryProvider* provider : std::as_const(m_providers)) {
			dictionary = provider->requestDictionary(language);
			if (dictionary && dictionary->isValid()) {
				break;
			} else {
				delete dictionary;
				dictionary = nullptr;
			}
		}

		if (!dictionary) {
			return DictionaryFallback::instance();
		}
		dictionary->addToSession(m_personal);
		m_dictionaries[language] = dictionary;
	}
	return &m_dictionaries[language];
}

//-----------------------------------------------------------------------------
