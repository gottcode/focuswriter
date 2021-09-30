/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_ABSTRACT_DICTIONARY_PROVIDER_H
#define FOCUSWRITER_ABSTRACT_DICTIONARY_PROVIDER_H

class AbstractDictionary;

#include <QStringList>

class AbstractDictionaryProvider
{
public:
	virtual ~AbstractDictionaryProvider()
	{
	}

	virtual bool isValid() const = 0;
	virtual QStringList availableDictionaries() const = 0;
	virtual AbstractDictionary* requestDictionary(const QString& language) const = 0;

	virtual void setIgnoreNumbers(bool ignore) = 0;
	virtual void setIgnoreUppercase(bool ignore) = 0;
};

#endif // FOCUSWRITER_ABSTRACT_DICTIONARY_PROVIDER_H
