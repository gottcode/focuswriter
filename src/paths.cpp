/*
	SPDX-FileCopyrightText: 2013-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "paths.h"

#include "daily_progress.h"
#include "dictionary_manager.h"
#include "document_cache.h"
#include "session.h"
#include "sound.h"
#include "symbols_model.h"
#include "theme.h"

#include <QDir>
#include <QFile>
#include <QIcon>
#include <QStandardPaths>

//-----------------------------------------------------------------------------

void Paths::load(const QString& appdir, QString& userdir, const QString& datadir)
{
	// Set locations of fallback icons
	QStringList paths = QIcon::themeSearchPaths();
	paths.prepend(datadir + "/icons");
	QIcon::setThemeSearchPaths(paths);

	// Set sounds path
	Sound::setPath(datadir + "/sounds");

	// Set unicode names path
	SymbolsModel::setPath(datadir + "/symbols1510.dat");

	// Find user data dir if not in portable mode
	if (userdir.isEmpty()) {
		userdir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

		// Migrate data from old location
		if (!QFile::exists(userdir)) {
			const QString oldpath = oldDataPath();
			if (!oldpath.isEmpty()) {
				QDir dir(userdir + "/../");
				dir.mkpath(dir.absolutePath());
				dir.rename(oldpath, userdir);
			}
		}
	}

	// Create base user data path
	QDir dir(userdir);
	if (!dir.exists()) {
		dir.mkpath(dir.absolutePath());
	}

	// Set cache path
	if (!dir.exists("Cache/Files")) {
		dir.mkpath("Cache/Files");
	}
	DocumentCache::setPath(dir.absoluteFilePath("Cache/Files"));

	// Set sessions path
	if (!dir.exists("Sessions")) {
		dir.mkdir("Sessions");
	}
	Session::setPath(dir.absoluteFilePath("Sessions"));

	// Set themes path
	if (!dir.exists("Themes")) {
		if (dir.exists("themes")) {
			dir.rename("themes", "Themes");
		} else {
			dir.mkdir("Themes");
		}
	}
	if (!dir.exists("Themes/Images")) {
		dir.mkdir("Themes/Images");
	}
	if (!dir.exists("Themes/Previews/Default")) {
		dir.mkpath("Themes/Previews/Default");
	}
	Theme::setDefaultPath(datadir + "/themes");
	Theme::setPath(dir.absoluteFilePath("Themes"));

	// Set dictionary paths
	if (!dir.exists("Dictionaries")) {
		if (dir.exists("dictionaries")) {
			dir.rename("dictionaries", "Dictionaries");
		} else {
			dir.mkdir("Dictionaries");
		}
	}
	DictionaryManager::setPath(dir.absoluteFilePath("Dictionaries"));

	QDir::setSearchPaths("dict", {
		DictionaryManager::path()
#ifdef Q_OS_WIN
		, appdir + "/dictionaries"
#endif
	});

	// Set location for daily progress
	DailyProgress::setPath(dir.absoluteFilePath("DailyProgress.ini"));
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
