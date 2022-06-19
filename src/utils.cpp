/*
	SPDX-FileCopyrightText: 2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "utils.h"

#include <QFile>

//-----------------------------------------------------------------------------

bool compareFiles(const QString& filename1, const QString& filename2)
{
	// Compare sizes
	QFile file1(filename1);
	QFile file2(filename2);
	if (file1.size() != file2.size()) {
		return false;
	}

	// Compare contents
	bool equal = true;
	if (file1.open(QFile::ReadOnly) && file2.open(QFile::ReadOnly)) {
		while (!file1.atEnd()) {
			if (file1.read(1000) != file2.read(1000)) {
				equal = false;
				break;
			}
		}
		file1.close();
		file2.close();
	} else {
		equal = false;
	}
	return equal;
}

//-----------------------------------------------------------------------------

bool localeAwareSort(const QString& lhs, const QString& rhs)
{
	return QString::localeAwareCompare(lhs, rhs) < 0;
}

//-----------------------------------------------------------------------------

QStringList splitStringAtLastNumber(const QString& string)
{
	QStringList result;
	result.append(string);
	result.append(QString::number(1));

	int index = string.length() - 1;
	if (string.at(index).isDigit()) {
		while (string.at(index).isDigit()) {
			--index;
		}
		++index;
		result[0].truncate(index);
		result[1] = string.mid(index);
	} else {
		result[0] += " ";
	}

	return result;
}

//-----------------------------------------------------------------------------
