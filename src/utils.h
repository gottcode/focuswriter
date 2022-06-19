/*
	SPDX-FileCopyrightText: 2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_UTILS_H
#define FOCUSWRITER_UTILS_H

#include <QString>

bool compareFiles(const QString& filename1, const QString& filename2);
bool localeAwareSort(const QString& lhs, const QString& rhs);
QStringList splitStringAtLastNumber(const QString& string);

#endif // FOCUSWRITER_UTILS_H
