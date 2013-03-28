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

#include "dictionary.h"

#include "dictionary_data.h"
#include "dictionary_manager.h"
#include "../smart_quotes.h"

#include <QStringList>
#include <QTextCodec>

#include <hunspell.hxx>

//-----------------------------------------------------------------------------

static bool f_ignore_numbers = false;
static bool f_ignore_uppercase = true;

//-----------------------------------------------------------------------------

Dictionary::Dictionary(DictionaryData** data) :
	d((data && *data && (*data)->dictionary()) ? data : 0)
{
}

//-----------------------------------------------------------------------------

QStringRef Dictionary::check(const QString& string, int start_at) const
{
	if (!d) {
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
				if (!(*d)->dictionary()->spell((*d)->codec()->fromUnicode(word).constData())) {
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

QStringList Dictionary::suggestions(const QString& word) const
{
	QStringList result;
	if (!d) {
		return result;
	}

	QString check = word;
	check.replace(QChar(0x2019), QLatin1Char('\''));
	char** suggestions = 0;
	int count = (*d)->dictionary()->suggest(&suggestions, (*d)->codec()->fromUnicode(check).constData());
	if (suggestions != 0) {
		for (int i = 0; i < count; ++i) {
			QString word = (*d)->codec()->toUnicode(suggestions[i]);
			if (SmartQuotes::isEnabled()) {
				SmartQuotes::replace(word);
			}
			result.append(word);
		}
		(*d)->dictionary()->free_list(&suggestions, count);
	}
	return result;
}

//-----------------------------------------------------------------------------

void Dictionary::addWord(const QString& word)
{
	DictionaryManager::instance().add(word);
}

//-----------------------------------------------------------------------------

void Dictionary::setIgnoreNumbers(bool ignore)
{
	f_ignore_numbers = ignore;
}

//-----------------------------------------------------------------------------

void Dictionary::setIgnoreUppercase(bool ignore)
{
	f_ignore_uppercase = ignore;
}

//-----------------------------------------------------------------------------
