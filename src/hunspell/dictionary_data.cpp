/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_data.h"

#include "dictionary_manager.h"
#include "../smart_quotes.h"

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextCodec>

#include <hunspell.hxx>

//-----------------------------------------------------------------------------

static bool f_ignore_numbers = false;
static bool f_ignore_uppercase = true;

//-----------------------------------------------------------------------------

DictionaryData::DictionaryData(const QString& language) :
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
	m_dictionary = new Hunspell(QFile::encodeName(aff).constData(), QFile::encodeName(dic).constData());
	m_codec = QTextCodec::codecForName(m_dictionary->get_dic_encoding());
	if (!m_codec) {
		delete m_dictionary;
		m_dictionary = 0;
	}
}

//-----------------------------------------------------------------------------

DictionaryData::~DictionaryData()
{
	delete m_dictionary;
}

//-----------------------------------------------------------------------------

QStringRef DictionaryData::check(const QString& string, int start_at) const
{
	if (!m_dictionary) {
		return QStringRef();
	}

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
				if (c == 0x0027 || c == 0x2019) {
					chars++;
					break;
				}

			default:
				if (index != -1) {
					is_word = true;
				}
				break;
		}

		if (is_word || (i == count && index != -1)) {
			if (!is_uppercase && !is_number) {
				QStringRef check(&string, index, length);
				QString word = check.toString();
				word.replace(QChar(0x2019), QLatin1Char('\''));
				if (!m_dictionary->spell(m_codec->fromUnicode(word).constData())) {
					return check;
				}
			}
			index = -1;
			is_word = false;
			is_number = false;
			is_uppercase = f_ignore_uppercase;
		}
	}

	return QStringRef();
}

//-----------------------------------------------------------------------------

QStringList DictionaryData::suggestions(const QString& word) const
{
	QStringList result;
	if (!m_dictionary) {
		return result;
	}

	QString check = word;
	check.replace(QChar(0x2019), QLatin1Char('\''));
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
	return result;
}

//-----------------------------------------------------------------------------

void DictionaryData::addToPersonal(const QString& word)
{
	DictionaryManager::instance().add(word);
}

//-----------------------------------------------------------------------------

void DictionaryData::addToSession(const QStringList& words)
{
	if (m_dictionary) {
		foreach (const QString& word, words) {
			m_dictionary->add(m_codec->fromUnicode(word).constData());
		}
	}
}

//-----------------------------------------------------------------------------

void DictionaryData::removeFromSession(const QStringList& words)
{
	if (m_dictionary) {
		foreach (const QString& word, words) {
			m_dictionary->remove(m_codec->fromUnicode(word).constData());
		}
	}
}

//-----------------------------------------------------------------------------

void DictionaryData::setIgnoreNumbers(bool ignore)
{
	f_ignore_numbers = ignore;
}

//-----------------------------------------------------------------------------

void DictionaryData::setIgnoreUppercase(bool ignore)
{
	f_ignore_uppercase = ignore;
}

//-----------------------------------------------------------------------------
