/***********************************************************************
 *
 * Copyright (C) 2008-2009 Graeme Gott <graeme@gottcode.org>
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

#include "theme.h"
#include "window.h"

#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("FocusWriter");
	app.setOrganizationDomain("gottcode.org");
	app.setOrganizationName("GottCode");

	QTranslator qt_translator;
	qt_translator.load("qt_" + QLocale::system().name());
	app.installTranslator(&qt_translator);

	QTranslator translator;
	translator.load("focuswriter_" + QLocale::system().name());
	app.installTranslator(&translator);

	// Create theme path
#if defined(Q_OS_MAC)
	QString path = QDir::homePath() + "/Library/Application Support/GottCode/FocusWriter/Themes";
#elif defined(Q_OS_UNIX)
	QString path = qgetenv("$XDG_DATA_HOME");
	if (path.isEmpty()) {
		path = QDir::homePath() + "/.local/share";
	}
	path += "/focuswriter/themes";
#elif defined(Q_OS_WIN32)
	QString path = QDir::homePath() + "/Application Data/GottCode/FocusWriter/Themes";
#endif
	if (!QFile::exists(path)) {
		QDir dir = QDir::home();
		dir.mkpath(path);
	}
	Theme::setPath(path);

	// Create theme from old settings
	if (QDir(path, "*.theme").entryList(QDir::Files).isEmpty()) {
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
	path = QSettings().value("Save/Location", QDir::homePath() + "/FocusWriter").toString();
	if (!QFile::exists(path)) {
		QDir dir = QDir::home();
		dir.mkpath(path);
	}
	QDir::setCurrent(path);

	// Load current daily progress
	QSettings settings;
	if (settings.value("Progress/Date").toDate() != QDate::currentDate()) {
		settings.remove("Progress");
	}
	settings.setValue("Progress/Date", QDate::currentDate().toString(Qt::ISODate));
	int current_wordcount = settings.value("Progress/Words", 0).toInt();
	int current_time = settings.value("Progress/Time", 0).toInt();

	// Open last saved document
	Window window(current_wordcount, current_time);
	QString filename = QSettings().value("Save/Current").toString();
	if (!filename.isEmpty()) {
		window.open(filename);
	}

	return app.exec();
}
