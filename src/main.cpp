/*
	SPDX-FileCopyrightText: 2008-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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
#include "window.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QSettings>
#include <QStringList>

int main(int argc, char** argv)
{
	Application app(argc, argv);
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

	// Handle commandline
	QCommandLineParser parser;
	parser.setApplicationDescription(Window::tr("A simple fullscreen word processor"));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("files", QCoreApplication::translate("main", "Files to open in current session."), "[files]");
	parser.process(app);
	const QStringList files = parser.positionalArguments();

	if (app.isRunning()) {
		app.sendMessage(files.join(QLatin1String("\n")));
		return 0;
	}

	// Find user data dir if not in portable mode
	if (userdir.isEmpty()) {
		userdir = Paths::dataPath();
		if (!QFile::exists(userdir)) {
			QDir dir(userdir);
			dir.mkpath(dir.absolutePath());

			// Migrate data from old location
			QString oldpath = Paths::oldDataPath();
			if (QFile::exists(oldpath)) {
				QStringList old_dirs{ QString() };

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

	for (const QString& datadir : qAsConst(datadirs)) {
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
	if (!app.createWindow(files)) {
		return 0;
	}

	return app.exec();
}
