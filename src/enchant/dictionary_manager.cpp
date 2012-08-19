/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "dictionary_manager.h"

#include "dictionary.h"
#include "dictionary_data.h"
#include "../smart_quotes.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

//-----------------------------------------------------------------------------

namespace
{
	bool compareWords(const QString& s1, const QString& s2)
	{
		return s1.localeAwareCompare(s2) < 0;
	}

	QStringList f_languages;
	void foundLanguage(const char* const lang, const char* const provider_name, const char* const provider_desc , const char* const provider_file, void*)
	{
		Q_UNUSED(provider_name)
		Q_UNUSED(provider_desc)
		Q_UNUSED(provider_file)
		QString language = QString::fromUtf8(lang);
		if (!f_languages.contains(language)) {
			f_languages.append(language);
		}
	}
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
	f_languages.clear();
	enchant_broker_list_dicts(m_broker, &foundLanguage, 0);
	return f_languages;
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

Dictionary DictionaryManager::requestDictionary(const QString& language)
{
	if (language.isEmpty()) {
		// Fetch shared default dictionary
		if (!m_default_dictionary) {
			m_default_dictionary = *requestDictionaryData(m_default_language);
		}
		return &m_default_dictionary;
	} else {
		// Fetch specific dictionary
		return requestDictionaryData(language);
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
	emit changed();
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
	qSort(personal.begin(), personal.end(), compareWords);
	if (personal == m_personal) {
		return;
	}

	// Remove current personal dictionary
	foreach (DictionaryData* dictionary, m_dictionaries) {
		dictionary->removeFromSession(m_personal);
	}

	// Update and store personal dictionary
	m_personal = personal;
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		foreach (const QString& word, m_personal) {
			stream << word << "\n";
		}
	}

	// Add personal dictionary
	foreach (DictionaryData* dictionary, m_dictionaries) {
		dictionary->addToSession(m_personal);
	}

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

DictionaryManager::DictionaryManager()
{
#ifdef Q_OS_WIN
	// Add path for Voikko dictionary
	{
		QString path = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/dictionaries/");
#ifdef UNICODE
		SetEnvironmentVariable(L"VOIKKO_DICTIONARY_PATH", path.toStdWString().c_str());
#else
		SetEnvironmentVariable("VOIKKO_DICTIONARY_PATH", path.toLocal8Bit());
#endif
	}
#endif

	// Create dictionary broker
	m_broker = enchant_broker_init();

	// Add paths for Hunspell dictionaries
	QByteArray paths;
#ifdef Q_OS_WIN
	char sep = ';';
#else
	char sep = ':';
#endif
	QStringList locations = QDir::searchPaths("dict");
	int count = locations.count();
	for (int i = 0; i < count; ++i) {
		paths += QFile::encodeName(QDir::toNativeSeparators(locations.at(i)));
		if (i < (count - 1)) {
			paths += sep;
		}
	}
	enchant_broker_set_param(m_broker, "enchant.myspell.dictionary.path", paths.constData());

	// Load personal dictionary
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		while (!stream.atEnd()) {
			m_personal.append(stream.readLine());
		}
		qSort(m_personal.begin(), m_personal.end(), compareWords);
	}
}

//-----------------------------------------------------------------------------

DictionaryManager::~DictionaryManager()
{
	foreach (DictionaryData* dictionary, m_dictionaries) {
		delete dictionary;
	}
	m_dictionaries.clear();

	enchant_broker_free(m_broker);
}

//-----------------------------------------------------------------------------

DictionaryData** DictionaryManager::requestDictionaryData(const QString& language)
{
	if (!m_dictionaries.contains(language)) {
		DictionaryData* dictionary = new DictionaryData(m_broker, language);
		dictionary->addToSession(m_personal);
		m_dictionaries[language] = dictionary;
	}
	return &m_dictionaries[language];
}

//-----------------------------------------------------------------------------
