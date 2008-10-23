/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "window.h"

#include "preferences.h"

#include <QApplication>
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

	// Browse to documents
	QString path = QSettings().value("Save/Location", QDir::homePath() + "/FocusWriter").toString();
	if (!QFile::exists(path)) {
		QDir dir = QDir::home();
		dir.mkpath(path);
	}
	QDir::setCurrent(path);

	// Create preferences window
	Preferences* preferences = new Preferences;

	// Open last saved document
	Window window(preferences);
	QString filename = QSettings().value("Save/Current").toString();
	if (!filename.isEmpty()) {
		window.open(filename);
	}

	return app.exec();
}
