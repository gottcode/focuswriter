/*
	SPDX-FileCopyrightText: 2009-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "dictionary_provider_hunspell.h"

#include "abstract_dictionary.h"
#include "dictionary_manager.h"
#include "smart_quotes.h"
#include "text_codec.h"
#include "word_ref.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include <hunspell.hxx>

//-----------------------------------------------------------------------------

static bool f_ignore_numbers = false;
static bool f_ignore_uppercase = true;

//-----------------------------------------------------------------------------

namespace
{

class DictionaryHunspell : public AbstractDictionary
{
public:
	DictionaryHunspell(const QString& language);
	~DictionaryHunspell();

	bool isValid() const override
	{
		return m_dictionary;
	}

	WordRef check(const QString& string, int start_at) const override;
	QStringList suggestions(const QString& word) const override;

	void addToPersonal(const QString& word) override;
	void addToSession(const QStringList& words) override;
	void removeFromSession(const QStringList& words) override;

private:
	Hunspell* m_dictionary;
	TextCodec* m_codec;
};

//-----------------------------------------------------------------------------

DictionaryHunspell::DictionaryHunspell(const QString& language)
	: m_dictionary(nullptr)
	, m_codec(nullptr)
{
	// Find dictionary files
	QString aff = QFileInfo("dict:" + language + ".aff").absoluteFilePath();
	if (aff.isEmpty()) {
		aff = QFileInfo("dict:" + language + ".aff.hz").absoluteFilePath();
		aff.chop(3);
	}
	QString dic = QFileInfo("dict:" + language + ".dic").absoluteFilePath();
	if (dic.isEmpty()) {
		dic = QFileInfo("dict:" + language + ".dic.hz").absoluteFilePath();
		dic.chop(3);
	}
	if (language.isEmpty() || aff.isEmpty() || dic.isEmpty()) {
		return;
	}

	// Create dictionary
#ifndef Q_WIN32
	m_dictionary = new Hunspell(QFile::encodeName(aff).constData(), QFile::encodeName(dic).constData());
#else
	m_dictionary = new Hunspell( ("\\\\?\\" + QDir::toNativeSeparators(aff)).toUtf8().constData(),
			("\\\\?\\" + QDir::toNativeSeparators(dic)).toUtf8().constData() );
#endif
	m_codec = TextCodec::codecForName(m_dictionary->get_dic_encoding());
	if (!m_codec) {
		delete m_dictionary;
		m_dictionary = nullptr;
	}
}

//-----------------------------------------------------------------------------

DictionaryHunspell::~DictionaryHunspell()
{
	delete m_dictionary;
}

//-----------------------------------------------------------------------------

WordRef DictionaryHunspell::check(const QString& string, int start_at) const
{
	int index = -1;
	int length = 0;
	int chars = 1;
	bool is_number = false;
	bool is_uppercase = f_ignore_uppercase;
	bool is_word = false;

	const int count = string.length() - 1;
	for (int i = start_at; i <= count; ++i) {
		const QChar c = string.at(i);
		switch (c.category()) {
			case QChar::Number_DecimalDigit:
			case QChar::Number_Letter:
			case QChar::Number_Other:
				is_number = f_ignore_numbers;
				goto Letter;
			case QChar::Letter_Lowercase:
				is_uppercase = false;
				goto Letter;
			Letter:
			case QChar::Letter_Uppercase:
			case QChar::Letter_Titlecase:
			case QChar::Letter_Modifier:
			case QChar::Letter_Other:
			case QChar::Mark_NonSpacing:
			case QChar::Mark_SpacingCombining:
			case QChar::Mark_Enclosing:
				if (index == -1) {
					index = i;
					chars = 1;
					length = 0;
				}
				length += chars;
				chars = 1;
				break;

			case QChar::Punctuation_FinalQuote:
			case QChar::Punctuation_Other:
				if (c == '\'' || c == u'’') {
					chars++;
					break;
				}
				goto NotWord;

			NotWord:
			default:
				if (index != -1) {
					is_word = true;
				}
				break;
		}

		if (is_word || (i == count && index != -1)) {
			if (!is_uppercase && !is_number) {
				QString word = string.mid(index, length);
				word.replace(QChar(0x2019), QLatin1Char('\''));
#ifdef H_DEPRECATED
				if (!m_dictionary->spell(m_codec->fromUnicode(word).toStdString())) {
#else
				if (!m_dictionary->spell(m_codec->fromUnicode(word).constData())) {
#endif
					return WordRef(index, length);
				}
			}
			index = -1;
			is_word = false;
			is_number = false;
			is_uppercase = f_ignore_uppercase;
		}
	}

	return WordRef();
}

//-----------------------------------------------------------------------------

QStringList DictionaryHunspell::suggestions(const QString& word) const
{
	QStringList result;
	QString check = word;
	check.replace(QChar(0x2019), QLatin1Char('\''));
#ifdef H_DEPRECATED
	const std::vector<std::string> suggestions = m_dictionary->suggest(m_codec->fromUnicode(check).toStdString());
	for (const std::string& suggestion : suggestions) {
		QString word = m_codec->toUnicode(suggestion.c_str());
		if (SmartQuotes::isEnabled()) {
			SmartQuotes::replace(word);
		}
		result.append(word);
	}
#else
	char** suggestions = nullptr;
	const int count = m_dictionary->suggest(&suggestions, m_codec->fromUnicode(check).constData());
	if (suggestions) {
		for (int i = 0; i < count; ++i) {
			QString word = m_codec->toUnicode(suggestions[i]);
			if (SmartQuotes::isEnabled()) {
				SmartQuotes::replace(word);
			}
			result.append(word);
		}
		m_dictionary->free_list(&suggestions, count);
	}
#endif
	return result;
}

//-----------------------------------------------------------------------------

void DictionaryHunspell::addToPersonal(const QString& word)
{
	DictionaryManager::instance().add(word);
}

//-----------------------------------------------------------------------------

void DictionaryHunspell::addToSession(const QStringList& words)
{
	for (const QString& word : words) {
		m_dictionary->add(m_codec->fromUnicode(word).constData());
	}
}

//-----------------------------------------------------------------------------

void DictionaryHunspell::removeFromSession(const QStringList& words)
{
	for (const QString& word : words) {
		m_dictionary->remove(m_codec->fromUnicode(word).constData());
	}
}

}

//-----------------------------------------------------------------------------

DictionaryProviderHunspell::DictionaryProviderHunspell()
{
	QStringList dictdirs = QDir::searchPaths("dict");

	dictdirs += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "hunspell", QStandardPaths::LocateDirectory);
	dictdirs += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "myspell/dicts", QStandardPaths::LocateDirectory);
	dictdirs += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "myspell", QStandardPaths::LocateDirectory);

	QDir::setSearchPaths("dict", dictdirs);
}

//-----------------------------------------------------------------------------

QStringList DictionaryProviderHunspell::availableDictionaries() const
{
	QStringList result;
	const QStringList locations = QDir::searchPaths("dict");
	for (const QString& location : locations) {
		const QDir dir(location);

		QStringList dic_files = dir.entryList({ "*.dic*" }, QDir::Files, QDir::Name | QDir::IgnoreCase);
		dic_files.replaceInStrings(QRegularExpression("\\.dic.*"), "");
		QStringList aff_files = dir.entryList({ "*.aff*" }, QDir::Files);
		aff_files.replaceInStrings(QRegularExpression("\\.aff.*"), "");

		for (const QString& language : std::as_const(dic_files)) {
			if (aff_files.contains(language) && !result.contains(language)) {
				result.append(language);
			}
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

AbstractDictionary* DictionaryProviderHunspell::requestDictionary(const QString& language) const
{
	return new DictionaryHunspell(language);
}

//-----------------------------------------------------------------------------

void DictionaryProviderHunspell::setIgnoreNumbers(bool ignore)
{
	f_ignore_numbers = ignore;
}

//-----------------------------------------------------------------------------

void DictionaryProviderHunspell::setIgnoreUppercase(bool ignore)
{
	f_ignore_uppercase = ignore;
}

//-----------------------------------------------------------------------------
