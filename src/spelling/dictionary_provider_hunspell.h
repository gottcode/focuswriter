/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DICTIONARY_PROVIDER_HUNSPELL_H
#define DICTIONARY_PROVIDER_HUNSPELL_H

#include "abstract_dictionary_provider.h"

class DictionaryProviderHunspell : public AbstractDictionaryProvider
{
public:
	DictionaryProviderHunspell();

	bool isValid() const
	{
		return true;
	}

	QStringList availableDictionaries() const;
	AbstractDictionary* requestDictionary(const QString& language) const;

	void setIgnoreNumbers(bool ignore);
	void setIgnoreUppercase(bool ignore);
};

#endif
