/*
	SPDX-FileCopyrightText: 2013-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "paths.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>

//-----------------------------------------------------------------------------

QString Paths::dataPath()
{
	static QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	return path;
}

//-----------------------------------------------------------------------------

QString Paths::oldDataPath()
{
	QStringList oldpaths;
	QString oldpath;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	// Data path from Qt 4 version of 1.4
	oldpaths.append(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/GottCode/FocusWriter");
#endif

	// Data path from 1.0
#if defined(Q_OS_MAC)
	oldpath = QDir::homePath() + "/Library/Application Support/GottCode/FocusWriter/";
#elif defined(Q_OS_UNIX)
	oldpath = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
	if (oldpath.isEmpty()) {
		oldpath = QDir::homePath() + "/.local/share";
	}
	oldpath += "/focuswriter";
#else
	oldpath = QDir::homePath() + "/Application Data/GottCode/FocusWriter/";
#endif
	if (!oldpaths.contains(oldpath)) {
		oldpaths.append(oldpath);
	}

	// Check if an old data location exists
	oldpath.clear();
	for (const QString& testpath : oldpaths) {
		if (QFile::exists(testpath)) {
			oldpath = testpath;
			break;
		}
	}

	return oldpath;
}

//-----------------------------------------------------------------------------
