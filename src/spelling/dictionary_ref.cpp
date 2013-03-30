/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_ref.h"

//-----------------------------------------------------------------------------

namespace
{

class DictionaryFallback : public AbstractDictionary
{
public:
	bool isValid() const
	{
		return true;
	}

	QStringRef check(const QString& string, int start_at) const
	{
		Q_UNUSED(string);
		Q_UNUSED(start_at);
		return QStringRef();
	}

	QStringList suggestions(const QString& word) const
	{
		Q_UNUSED(word);
		return QStringList();
	}

	void addToPersonal(const QString& word)
	{
		Q_UNUSED(word);
	}

	void addToSession(const QStringList& words)
	{
		Q_UNUSED(words);
	}

	void removeFromSession(const QStringList& words)
	{
		Q_UNUSED(words);
	}
} f_fallback;

}

static AbstractDictionary* f_fallback_ptr = &f_fallback;

//-----------------------------------------------------------------------------

DictionaryRef::DictionaryRef(AbstractDictionary** data) :
	d((data && *data) ? data : &f_fallback_ptr)
{
}

//-----------------------------------------------------------------------------
