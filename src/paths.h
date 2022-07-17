/*
	SPDX-FileCopyrightText: 2013-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_PATHS_H
#define FOCUSWRITER_PATHS_H

#include <QString>

class Paths
{
public:
	static void load(const QString& appdir, QString& userdir, const QString& datadir);

private:
	static QString oldDataPath();
};

#endif // FOCUSWRITER_PATHS_H
