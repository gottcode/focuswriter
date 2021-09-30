/*
	SPDX-FileCopyrightText: 2011-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

Application::Application(int& argc, char** argv)
	: QtSingleApplication("org.gottcode.FocusWriter", argc, argv)
	, m_window(0)
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

#ifndef Q_OS_MAC
	setAttribute(Qt::AA_DontUseNativeMenuBar);
#else
	setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

#ifdef RTFCLIPBOARD
	new RTF::Clipboard;
#endif

	qputenv("UNICODEMAP_JP", "cp932");

	processEvents();
}

//-----------------------------------------------------------------------------

bool Application::createWindow(const QStringList& files)
{
	m_files = files;
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
