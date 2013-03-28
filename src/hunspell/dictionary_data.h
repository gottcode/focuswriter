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

class Hunspell;

class QString;
class QStringList;
class QStringRef;
class QTextCodec;

class DictionaryData
{
public:
	DictionaryData(const QString& language);
	~DictionaryData();

	QTextCodec* codec() const;
	Hunspell* dictionary() const;

	void addToSession(const QStringList& words);
	void removeFromSession(const QStringList& words);

private:
	Hunspell* m_dictionary;
	QTextCodec* m_codec;
};

inline QTextCodec* DictionaryData::codec() const
{
	return m_codec;
}

inline Hunspell* DictionaryData::dictionary() const
{
	return m_dictionary;
}

#endif
