/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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
#include "document.h"
#include "locale_dialog.h"
#include "session.h"
#include "sound.h"
#include "theme.h"
#include "window.h"

#ifdef Q_OS_MAC
#include "rtf/clipboard_mac.h"
#endif
#ifdef Q_OS_WIN32
#include "rtf/clipboard_windows.h"
#endif

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QSettings>
#include <QStringList>

//-----------------------------------------------------------------------------

class Application : public QApplication
{
public:
	Application(int& argc, char** argv);

	void createWindow();

protected:
	virtual bool event(QEvent* e);

private:
	QStringList m_files;
	Window* m_window;
};

//-----------------------------------------------------------------------------

Application::Application(int& argc, char** argv)
	: QApplication(argc, argv),
	m_window(0)
{
	setApplicationName("FocusWriter");
	setApplicationVersion("1.3.6");
	setOrganizationDomain("gottcode.org");
	setOrganizationName("GottCode");
	{
		QIcon fallback(":/hicolor/256x256/apps/focuswriter.png");
		fallback.addFile(":/hicolor/128x128/apps/focuswriter.png");
		fallback.addFile(":/hicolor/64x64/apps/focuswriter.png");
		fallback.addFile(":/hicolor/48x48/apps/focuswriter.png");
		fallback.addFile(":/hicolor/32x32/apps/focuswriter.png");
		fallback.addFile(":/hicolor/24x24/apps/focuswriter.png");
		fallback.addFile(":/hicolor/22x22/apps/focuswriter.png");
		fallback.addFile(":/hicolor/16x16/apps/focuswriter.png");
		if (!QIcon::themeName().isEmpty() && (QIcon::themeName() != "hicolor")) {
			setWindowIcon(QIcon::fromTheme("focuswriter", fallback));
		} else {
			setWindowIcon(fallback);
		}
	}

#ifndef Q_WS_MAC
	setAttribute(Qt::AA_DontUseNativeMenuBar);
#else
	setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

# if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
	new RTF::Clipboard;
#endif

	qputenv("UNICODEMAP_JP", "cp932");

	m_files = arguments().mid(1);
	processEvents();
}

//-----------------------------------------------------------------------------

void Application::createWindow()
{
#ifndef Q_WS_MAC
	setAttribute(Qt::AA_DontShowIconsInMenus, !QSettings().value("Window/MenuIcons", false).toBool());
#endif
	m_window = new Window(m_files);
}

//-----------------------------------------------------------------------------

bool Application::event(QEvent* e)
{
	if (e->type() != QEvent::FileOpen) {
		return QApplication::event(e);
	} else {
		QString file = static_cast<QFileOpenEvent*>(e)->file();
		if (m_window) {
			m_window->addDocuments(QStringList(file), QStringList(file));
		} else {
			m_files.append(file);
		}
		e->accept();
		return true;
	}
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	Application app(argc, argv);
	QString appdir = app.applicationDirPath();

	QStringList paths = QIcon::themeSearchPaths();
	paths.prepend(appdir + "/../share/focuswriter/icons");
	paths.prepend(appdir + "/icons");
	QIcon::setThemeSearchPaths(paths);

	paths.clear();
	paths.append(appdir + "/sounds/");
	paths.append(appdir + "/../share/focuswriter/sounds/");
	paths.append(appdir + "/../Resources/sounds");
	foreach (const QString& path, paths) {
		if (QFile::exists(path)) {
			Sound::setPath(path);
			break;
		}
	}

	// Find data paths
	QStringList locations;
#if defined(Q_OS_MAC)
	QFileInfo portable(appdir + "/../../../Data");
	QString path = QDir::homePath() + "/Library/Application Support/GottCode/FocusWriter/";

	locations.append(QDir::homePath() + "/Library/Spelling");
	locations.append("/Library/Spelling");
	locations.append(appdir + "/../Resources/Dictionaries");
#elif defined(Q_OS_UNIX)
	QFileInfo portable(appdir + "/Data");
	QString path = qgetenv("XDG_DATA_HOME");
	if (path.isEmpty()) {
		path = QDir::homePath() + "/.local/share";
	}
	path += "/focuswriter/";

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
	QFileInfo portable(appdir + "/Data");
	QString path = QDir::homePath() + "/Application Data/GottCode/FocusWriter/";

	locations.append(appdir + "/Dictionaries");
#endif

	// Handle portability
	if (portable.exists() && portable.isWritable()) {
		path = portable.absoluteFilePath();
		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, path + "/Settings");
	}

	// Load application language
	LocaleDialog::loadTranslator("focuswriter_");

	// Create base data path
	QDir dir;
	if (!QFile::exists(path)) {
		dir.mkpath(path);
	}
	dir.setPath(path);

	// Create cache path
	if (!dir.exists("Cache/Files")) {
		dir.mkpath("Cache/Files");
	}
	Document::setCachePath(dir.filePath("Cache/Files"));

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
	Theme::setPath(dir.absoluteFilePath("Themes"));

	// Set dictionary paths
	if (!dir.exists("Dictionaries")) {
		if (dir.exists("dictionaries")) {
			dir.rename("dictionaries", "Dictionaries");
		} else {
			dir.mkdir("Dictionaries");
		}
	}
	Dictionary::setPath(dir.absoluteFilePath("Dictionaries"));
	locations.prepend(Dictionary::path());
	QDir::setSearchPaths("dict", locations);

	// Create theme from old settings
	if (QDir(Theme::path(), "*.theme").entryList(QDir::Files).isEmpty()) {
		QSettings settings;
		Theme theme;
		theme.setName(Session::tr("Default"));

		theme.setBackgroundType(settings.value("Background/Position", theme.backgroundType()).toInt());
		theme.setBackgroundColor(settings.value("Background/Color", theme.backgroundColor()).toString());
		theme.setBackgroundImage(settings.value("Background/Image").toString());
		settings.remove("Background");

		theme.setForegroundColor(settings.value("Page/Color", theme.foregroundColor()).toString());
		theme.setForegroundWidth(settings.value("Page/Width", theme.foregroundWidth()).toInt());
		theme.setForegroundOpacity(settings.value("Page/Opacity", theme.foregroundOpacity()).toInt());
		settings.remove("Page");

		theme.setTextColor(settings.value("Text/Color", theme.textColor()).toString());
		theme.setTextFont(settings.value("Text/Font", theme.textFont()).value<QFont>());
		settings.remove("Text");

		settings.setValue("ThemeManager/Theme", theme.name());
	}

	// Create main window
	app.createWindow();

	// Browse to documents after command-line specified documents have been loaded
	QDir::setCurrent(QSettings().value("Save/Location", QDir::homePath()).toString());

	return app.exec();
}
