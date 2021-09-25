/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DICTIONARY_PROVIDER_VOIKKO_H
#define FOCUSWRITER_DICTIONARY_PROVIDER_VOIKKO_H

#include "abstract_dictionary_provider.h"

class DictionaryProviderVoikko : public AbstractDictionaryProvider
{
public:
	DictionaryProviderVoikko();

	bool isValid() const;
	QStringList availableDictionaries() const;
	AbstractDictionary* requestDictionary(const QString& language) const;

	void setIgnoreNumbers(bool ignore);
	void setIgnoreUppercase(bool ignore);
};

#endif // FOCUSWRITER_DICTIONARY_PROVIDER_VOIKKO_H
