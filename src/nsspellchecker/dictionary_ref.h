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

#ifndef DICTIONARY_REF_H
#define DICTIONARY_REF_H

class DictionaryData;

class QString;
class QStringList;
class QStringRef;

class DictionaryRef
{
public:
	DictionaryRef(DictionaryData** data = 0) : d((data && *data) ? data : 0) { }

	QStringRef check(const QString& string, int start_at) const;
	QStringList suggestions(const QString& word) const;

	void addWord(const QString& word);

private:
	DictionaryData** d;
};

#endif
