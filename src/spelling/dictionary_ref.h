/*
	SPDX-FileCopyrightText: 2013-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DICTIONARY_REF_H
#define FOCUSWRITER_DICTIONARY_REF_H

#include "abstract_dictionary.h"
#include "word_ref.h"
class DictionaryManager;

class DictionaryRef
{
public:
	WordRef check(const QString& string, int start_at) const
	{
		return (*d)->check(string, start_at);
	}

	QStringList suggestions(const QString& word) const
	{
		return (*d)->suggestions(word);
	}

	void addToPersonal(const QString& word)
	{
		(*d)->addToPersonal(word);
	}

	friend class DictionaryManager;

private:
	explicit DictionaryRef(AbstractDictionary** data)
		: d(data)
	{
		Q_ASSERT(d);
	}

private:
	AbstractDictionary** d;
};

#endif // FOCUSWRITER_DICTIONARY_REF_H
