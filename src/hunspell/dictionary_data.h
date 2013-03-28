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

#ifndef DICTIONARY_DATA_H
#define DICTIONARY_DATA_H

#include "../abstract_dictionary.h"
class Hunspell;

class QString;
class QStringList;
class QStringRef;
class QTextCodec;

class DictionaryData : public AbstractDictionary
{
public:
	DictionaryData(const QString& language);
	~DictionaryData();

	QStringRef check(const QString& string, int start_at) const;
	QStringList suggestions(const QString& word) const;

	void addToPersonal(const QString& word);
	void addToSession(const QStringList& words);
	void removeFromSession(const QStringList& words);

	static void setIgnoreNumbers(bool ignore);
	static void setIgnoreUppercase(bool ignore);

private:
	Hunspell* m_dictionary;
	QTextCodec* m_codec;
};

#endif
