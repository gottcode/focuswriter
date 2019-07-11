/***********************************************************************
 *
 * Copyright (C) 2011, 2012, 2013, 2014, 2015, 2019 Graeme Gott <graeme@gottcode.org>
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

#ifdef RTFCLIPBOARD
  #ifdef Q_OS_MAC
	#include "clipboard_mac.h"
  #endif
  #ifdef Q_OS_WIN
	#include "clipboard_windows.h"
  #endif
#endif

//-----------------------------------------------------------------------------

Application::Application(int& argc, char** argv) :
	QtSingleApplication("org.gottcode.FocusWriter", argc, argv),
	m_window(0)
{
	setApplicationName("FocusWriter");
	setApplicationVersion(VERSIONSTR);
	setApplicationDisplayName(Window::tr("FocusWriter"));
	setOrganizationDomain("gottcode.org");
	setOrganizationName("GottCode");
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
	setWindowIcon(QIcon::fromTheme("focuswriter", QIcon(":/focuswriter.png")));
	setDesktopFileName("focuswriter");
#endif

	setAttribute(Qt::AA_UseHighDpiPixmaps, true);

#ifndef Q_OS_MAC
	setAttribute(Qt::AA_DontUseNativeMenuBar);
#else
	setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

#ifdef RTFCLIPBOARD
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
	connect(this, &Application::messageReceived, m_window, QOverload<const QString&>::of(&Window::addDocuments));

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
