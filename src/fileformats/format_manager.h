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

#ifndef FORMAT_MANAGER_H
#define FORMAT_MANAGER_H

class FormatReader;

#include <QCoreApplication>
#include <QString>
class QIODevice;
class QStringList;

class FormatManager
{
	Q_DECLARE_TR_FUNCTIONS(FormatManager)

public:
	static FormatReader* createReader(QIODevice* device, const QString& type = QString());
	static QString filter(const QString& type);
	static QStringList filters(const QString& type = QString());
	static bool isRichText(const QString& filename);
	static QStringList types();
};

#endif
