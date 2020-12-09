/***********************************************************************
 *
 * Copyright (C) 2009-2020 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_provider_hunspell.h"

#include "abstract_dictionary.h"
#include "dictionary_manager.h"
#include "smart_quotes.h"
#include "word_ref.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListIterator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextCodec>

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

	bool isValid() const
	{
		return m_dictionary;
	}

	WordRef check(const QString& string, int start_at) const;
	QStringList suggestions(const QString& word) const;

	void addToPersonal(const QString& word);
	void addToSession(const QStringList& words);
	void removeFromSession(const QStringList& words);

private:
	Hunspell* m_dictionary;
	QTextCodec* m_codec;
};

//-----------------------------------------------------------------------------

DictionaryHunspell::DictionaryHunspell(const QString& language) :
	m_dictionary(0),
	m_codec(0)
{
	// Find dictionary files
	QString aff = QFileInfo("dict:" + language + ".aff").canonicalFilePath();
	if (aff.isEmpty()) {
		aff = QFileInfo("dict:" + language + ".aff.hz").canonicalFilePath();
		aff.chop(3);
	}
	QString dic = QFileInfo("dict:" + language + ".dic").canonicalFilePath();
	if (dic.isEmpty()) {
		dic = QFileInfo("dict:" + language + ".dic.hz").canonicalFilePath();
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
	m_codec = QTextCodec::codecForName(m_dictionary->get_dic_encoding());
	if (!m_codec) {
		delete m_dictionary;
		m_dictionary = 0;
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

	int count = string.length() - 1;
	for (int i = start_at; i <= count; ++i) {
		QChar c = string.at(i);
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
				auto word = string.mid(index, length);
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
	const auto suggestions = m_dictionary->suggest(m_codec->fromUnicode(check).toStdString());
	for (const auto& suggestion : suggestions) {
		QString word = m_codec->toUnicode(suggestion.c_str());
		if (SmartQuotes::isEnabled()) {
			SmartQuotes::replace(word);
		}
		result.append(word);
	}
#else
	char** suggestions = 0;
	int count = m_dictionary->suggest(&suggestions, m_codec->fromUnicode(check).constData());
	if (suggestions != 0) {
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
	QStringList locations = QDir::searchPaths("dict");
	QListIterator<QString> i(locations);
	while (i.hasNext()) {
		QDir dir(i.next());

		QStringList dic_files = dir.entryList(QStringList() << "*.dic*", QDir::Files, QDir::Name | QDir::IgnoreCase);
		dic_files.replaceInStrings(QRegularExpression("\\.dic.*"), "");
		QStringList aff_files = dir.entryList(QStringList() << "*.aff*", QDir::Files);
		aff_files.replaceInStrings(QRegularExpression("\\.aff.*"), "");

		for (const QString& language : dic_files) {
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
