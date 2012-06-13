/***********************************************************************
 *
 * Copyright (C) 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "window.h"

#include <QFileOpenEvent>
#include <QSettings>

#ifdef Q_OS_MAC
#include "rtf/clipboard_mac.h"
#endif
#ifdef Q_OS_WIN32
#include "rtf/clipboard_windows.h"
#endif

//-----------------------------------------------------------------------------

Application::Application(int& argc, char** argv) :
	QtSingleApplication("org.gottcode.FocusWriter", argc, argv),
	m_window(0)
{
	setApplicationName("FocusWriter");
	setApplicationVersion(VERSIONSTR);
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

#ifndef Q_OS_MAC
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

bool Application::createWindow()
{
	if (isRunning()) {
		sendMessage(m_files.join(QLatin1String("\n")));
		return false;
	}

#ifndef Q_OS_MAC
	setAttribute(Qt::AA_DontShowIconsInMenus, !QSettings().value("Window/MenuIcons", false).toBool());
#endif
	m_window = new Window(m_files);
	setActivationWindow(m_window);
	connect(this, SIGNAL(messageReceived(QString)), m_window, SLOT(addDocuments(QString)));

	return true;
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
