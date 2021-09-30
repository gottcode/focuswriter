/*
	SPDX-FileCopyrightText: 2009-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_ABSTRACT_DICTIONARY_H
#define FOCUSWRITER_ABSTRACT_DICTIONARY_H

class WordRef;

#include <QStringList>

class AbstractDictionary
{
public:
	virtual ~AbstractDictionary()
	{
	}

	virtual bool isValid() const = 0;
	virtual WordRef check(const QString& string, int start_at) const = 0;
	virtual QStringList suggestions(const QString& word) const = 0;

	virtual void addToPersonal(const QString& word) = 0;
	virtual void addToSession(const QStringList& words) = 0;
	virtual void removeFromSession(const QStringList& words) = 0;
};

#endif // FOCUSWRITER_ABSTRACT_DICTIONARY_H
