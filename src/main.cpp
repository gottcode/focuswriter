/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2018 Graeme Gott <graeme@gottcode.org>
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

#include "application.h"
#include "daily_progress.h"
#include "dictionary_manager.h"
#include "document_cache.h"
#include "locale_dialog.h"
#include "paths.h"
#include "session.h"
#include "sound.h"
#include "symbols_model.h"
#include "theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QSettings>
#include <QStringList>

int main(int argc, char** argv)
{
#if !defined(Q_OS_MAC)
	if (!qEnvironmentVariableIsSet("QT_DEVICE_PIXEL_RATIO")
			&& !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
			&& !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
			&& !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")) {
		QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	}
#endif
	Application app(argc, argv);
	if (app.isRunning()) {
		app.sendMessage(app.files().join(QLatin1String("\n")));
		return 0;
	}
	QString appdir = app.applicationDirPath();

	// Allow passing Theme as signal parameter
	qRegisterMetaType<Theme>("Theme");

	// Find application data dirs
	QStringList datadirs;
#if defined(Q_OS_MAC)
	QFileInfo portable(appdir + "/../../../Data");
	datadirs.append(appdir + "/../Resources");
#elif defined(Q_OS_UNIX)
	QFileInfo portable(appdir + "/Data");
	datadirs.append(DATADIR);
	datadirs.append(appdir + "/../share/focuswriter");
#else
	QFileInfo portable(appdir + "/Data");
	datadirs.append(appdir);
#endif

	// Handle portability
	QString userdir;
	if (portable.exists() && portable.isWritable()) {
		userdir = portable.absoluteFilePath();
		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, userdir + "/Settings");
	}

	// Set locations of fallback icons
	{
		QStringList paths = QIcon::themeSearchPaths();
		for (const QString& path : datadirs) {
			paths.prepend(path + "/icons");
		}
		QIcon::setThemeSearchPaths(paths);
	}

	// Find sounds
	for (const QString& path : datadirs) {
		if (QFile::exists(path + "/sounds/")) {
			Sound::setPath(path + "/sounds/");
			break;
		}
	}

	// Find unicode names
	SymbolsModel::setData(datadirs);

	// Load application language
	LocaleDialog::loadTranslator("focuswriter_", datadirs);

	// Find user data dir if not in portable mode
	if (userdir.isEmpty()) {
		userdir = Paths::dataPath();
		if (!QFile::exists(userdir)) {
			QDir dir(userdir);
			dir.mkpath(dir.absolutePath());

			// Migrate data from old location
			QString oldpath = Paths::oldDataPath();
			if (QFile::exists(oldpath)) {
				QStringList old_dirs = QStringList() << "";

				QDir olddir(oldpath);
				for (int i = 0; i < old_dirs.count(); ++i) {
					QString subpath = old_dirs.at(i);
					dir.mkpath(userdir + "/" + subpath);
					olddir.setPath(oldpath + "/" + subpath);

					QStringList subdirs = olddir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
					for (const QString& subdir : subdirs) {
						old_dirs.append(subpath + "/" + subdir);
					}

					QStringList files = olddir.entryList(QDir::Files);
					for (const QString& file : files) {
						QFile::rename(olddir.absoluteFilePath(file), userdir + "/" + subpath + "/" + file);
					}
				}

				olddir.setPath(oldpath);
				for (int i = old_dirs.count() - 1; i >= 0; --i) {
					olddir.rmdir(oldpath + "/" + old_dirs.at(i));
				}
			}
		}
	}

	// Create base user data path
	QDir dir(userdir);
	if (!dir.exists()) {
		dir.mkpath(dir.absolutePath());
	}

	// Create cache path
	if (!dir.exists("Cache/Files")) {
		dir.mkpath("Cache/Files");
	}
	DocumentCache::setPath(dir.filePath("Cache/Files"));

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
	Theme::setPath(dir.absoluteFilePath("Themes"));

	for (const QString& datadir : datadirs) {
		QFileInfo info(datadir + "/themes");
		if (info.exists()) {
			Theme::setDefaultPath(info.absoluteFilePath());
			break;
		}
	}

	// Set dictionary paths
	if (!dir.exists("Dictionaries")) {
		if (dir.exists("dictionaries")) {
			dir.rename("dictionaries", "Dictionaries");
		} else {
			dir.mkdir("Dictionaries");
		}
	}
	DictionaryManager::setPath(dir.absoluteFilePath("Dictionaries"));
	QStringList dictdirs;
	dictdirs.append(DictionaryManager::path());
#ifdef Q_OS_WIN
	dictdirs.append(appdir + "/dictionaries");
#endif
	QDir::setSearchPaths("dict", dictdirs);

	// Set location for daily progress
	DailyProgress::setPath(dir.absoluteFilePath("DailyProgress.ini"));

	// Create theme from old settings
	if (QDir(Theme::path(), "*.theme").entryList(QDir::Files).isEmpty()) {
		QSettings settings;
		Theme theme(QString(), false);

		theme.setBackgroundType(settings.value("Background/Position", theme.backgroundType()).toInt());
		theme.setBackgroundColor(settings.value("Background/Color", theme.backgroundColor()).toString());
		theme.setBackgroundImage(settings.value("Background/Image").toString());
		settings.remove("Background");

		theme.setForegroundColor(settings.value("Page/Color", theme.foregroundColor()).toString());
		theme.setForegroundWidth(settings.value("Page/Width", theme.foregroundWidth().value()).toInt());
		theme.setForegroundOpacity(settings.value("Page/Opacity", theme.foregroundOpacity().value()).toInt());
		settings.remove("Page");

		theme.setTextColor(settings.value("Text/Color", theme.textColor()).toString());
		theme.setTextFont(settings.value("Text/Font", theme.textFont()).value<QFont>());
		settings.remove("Text");

		if (theme.isChanged()) {
			theme.saveChanges();
			settings.setValue("ThemeManager/Theme", theme.name());
		}
	}

	// Create main window
	if (!app.createWindow()) {
		return 0;
	}

	return app.exec();
}
