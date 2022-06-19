/*
	SPDX-FileCopyrightText: 2008-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "application.h"
#include "locale_dialog.h"
#include "paths.h"
#include "theme.h"
#include "window.h"

#include <QCommandLineParser>
#include <QDir>
#include <QSettings>

int main(int argc, char** argv)
{
	Application app(argc, argv);

	// Allow passing Theme as signal parameter
	qRegisterMetaType<Theme>("Theme");

	// Find application data dirs
	const QString appdir = app.applicationDirPath();
	const QStringList datadirs{
#if defined(Q_OS_MAC)
		appdir + "/../Resources"
#elif defined(Q_OS_UNIX)
		DATADIR,
		appdir + "/../share/focuswriter"
#else
		appdir
#endif
	};

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

	// Load paths
	Paths::load(appdir, datadirs);

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
