/***********************************************************************
 *
 * Copyright (C) 2014 Graeme Gott <graeme@gottcode.org>
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

#include "utils.h"

#include <QFile>
#include <QString>
#include <QStringList>

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
