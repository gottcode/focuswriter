/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary.h"
#include "theme.h"
#include "window.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("FocusWriter");
	app.setApplicationVersion("1.2.0");
	app.setOrganizationDomain("gottcode.org");
	app.setOrganizationName("GottCode");

	QTranslator qt_translator;
	qt_translator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qt_translator);

	QTranslator translator;
	translator.load(":/focuswriter_" + QLocale::system().name());
	app.installTranslator(&translator);

	// Find data paths
	QStringList locations;
#if defined(Q_OS_MAC)
	app.setAttribute(Qt::AA_DontShowIconsInMenus);

	QString path = QDir::homePath() + "/Library/Application Support/GottCode/FocusWriter/";
	QString themes = "Themes";
	QString dictionaries = "Dictionaries";

	locations.append(QCoreApplication::applicationDirPath() + "/../Resources/Dictionaries");
	locations.append("/Library/Application Support/GottCode/FocusWriter/Dictionaries");
#elif defined(Q_OS_UNIX)
	QString path = qgetenv("XDG_DATA_HOME");
	if (path.isEmpty()) {
		path = QDir::homePath() + "/.local/share";
	}
	path += "/focuswriter/";
	QString themes = "themes";
	QString dictionaries = "dictionaries";

	QStringList xdg = QString(qgetenv("XDG_DATA_DIRS")).split(QChar(':'), QString::SkipEmptyParts);
	if (xdg.isEmpty()) {
		xdg.append("/usr/local/share");
		xdg.append("/usr/share");
	}
	QStringList subdirs = QStringList() << "/hunspell" << "/myspell/dicts" << "/myspell";
	foreach (const QString& subdir, subdirs) {
		foreach (const QString& dir, xdg) {
			QString path = dir + subdir;
			if (!locations.contains(path)) {
				locations.append(path);
			}
		}
	}
#elif defined(Q_OS_WIN32)
	QString path = QDir::homePath() + "/Application Data/GottCode/FocusWriter/";
	QString themes = "Themes";
	QString dictionaries = "Dictionaries";

	locations.append(QCoreApplication::applicationDirPath() + "/Dictionaries");
#endif
	QDir dir = QDir::home();
	if (!QFile::exists(path)) {
		dir.mkpath(path);
	}
	dir.setPath(path);

	// Set themes path
	if (!dir.exists(themes)) {
		dir.mkdir(themes);
	}
	Theme::setPath(dir.filePath(themes));

	// Set dictionary paths
	if (!dir.exists(dictionaries)) {
		dir.mkdir(dictionaries);
	}
	Dictionary::setPath(dir.filePath(dictionaries));
	locations.prepend(Dictionary::path());
	QDir::setSearchPaths("dict", locations);

	// Create theme from old settings
	if (QDir(Theme::path(), "*.theme").entryList(QDir::Files).isEmpty()) {
		QSettings settings;
		Theme theme;

		theme.setBackgroundType(settings.value("Background/Position", 0).toInt());
		theme.setBackgroundColor(settings.value("Background/Color", "#cccccc").toString());
		theme.setBackgroundImage(settings.value("Background/Image").toString());
		settings.remove("Background");

		theme.setForegroundColor(settings.value("Page/Color", "#cccccc").toString());
		theme.setForegroundWidth(settings.value("Page/Width", 700).toInt());
		theme.setForegroundOpacity(settings.value("Page/Opacity", 100).toInt());
		settings.remove("Page");

		theme.setTextColor(settings.value("Text/Color", "#000000").toString());
		theme.setTextFont(settings.value("Text/Font").toString());
		settings.remove("Text");

		settings.setValue("ThemeManager/Theme", theme.name());
	}

	// Browse to documents
	QDir::setCurrent(QSettings().value("Save/Location", QDir::homePath()).toString());

	// Create main window
	new Window;

	return app.exec();
}
