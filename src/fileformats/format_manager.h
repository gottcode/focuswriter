/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_FORMAT_MANAGER_H
#define FOCUSWRITER_FORMAT_MANAGER_H

class FormatReader;

#include <QCoreApplication>
#include <QString>
class QIODevice;

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

#endif // FOCUSWRITER_FORMAT_MANAGER_H
