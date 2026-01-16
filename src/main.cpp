/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "locale_dialog.h"
#include "paths.h"
#include "theme.h"
#include "window.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <KDSingleApplication>

#ifdef RTFCLIPBOARD
  #ifdef Q_OS_WIN
	#include "clipboard_windows.h"
  #endif
#endif

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	app.setApplicationName("FocusWriter");
	app.setApplicationVersion(VERSIONSTR);
	app.setApplicationDisplayName(Window::tr("FocusWriter"));
	app.setOrganizationDomain("gottcode.org");
	app.setOrganizationName("GottCode");
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
	app.setWindowIcon(QIcon::fromTheme("focuswriter", QIcon(":/focuswriter.png")));
	app.setDesktopFileName("focuswriter");
#endif

#ifndef Q_OS_MAC
	app.setAttribute(Qt::AA_DontUseNativeMenuBar);
	app.setAttribute(Qt::AA_DontShowIconsInMenus, !QSettings().value("Window/MenuIcons", false).toBool());
#else
	app.setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

#ifdef RTFCLIPBOARD
	RtfClipboard clipboard;
#endif

	// Allow passing Theme as signal parameter
	qRegisterMetaType<Theme>("Theme");

	// Find application data
	const QString appdir = app.applicationDirPath();
	const QString datadir = QDir::cleanPath(appdir + "/" + FOCUSWRITER_DATADIR);

	// Handle portability
	QString userdir;
#ifdef Q_OS_MAC
	const QFileInfo portable(appdir + "/../../../Data");
#else
	const QFileInfo portable(appdir + "/Data");
#endif
	if (portable.exists() && portable.isWritable()) {
		userdir = portable.absoluteFilePath();
		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, userdir + "/Settings");
	}

	// Load application language
	LocaleDialog::loadTranslator("focuswriter_", datadir);

	// Handle commandline
	QCommandLineParser parser;
	parser.setApplicationDescription(Window::tr("A simple fullscreen word processor"));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("files", QCoreApplication::translate("main", "Files to open in current session."), "[files]");
	parser.process(app);
	const QStringList files = parser.positionalArguments();

	// Force single instance
	KDSingleApplication kdsa("org.gottcode.FocusWriter");
	if (!kdsa.isPrimaryInstance()) {
		const QString list = files.join(QLatin1String("\n"));
		kdsa.sendMessage(list.toUtf8());
		return 0;
	}

	// Load paths
	Paths::load(appdir, userdir, datadir);

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
	Window* window = new Window(files);
	Window::connect(&kdsa, &KDSingleApplication::messageReceived, window, qOverload<const QByteArray&>(&Window::addDocuments), Qt::QueuedConnection);

	return app.exec();
}
