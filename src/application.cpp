/*
	SPDX-FileCopyrightText: 2011-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "application.h"

#include "window.h"

#include <QFileOpenEvent>
#include <QSettings>

//-----------------------------------------------------------------------------

Application::Application(int& argc, char** argv)
	: QtSingleApplication("org.gottcode.FocusWriter", argc, argv)
	, m_window(nullptr)
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

	processEvents(QEventLoop::ExcludeUserInputEvents);
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
	connect(this, &Application::messageReceived, m_window, qOverload<const QString&>(&Window::addDocuments));

	return true;
}

//-----------------------------------------------------------------------------

bool Application::event(QEvent* e)
{
	if (e->type() != QEvent::FileOpen) {
		return QApplication::event(e);
	} else {
		const QString file = static_cast<QFileOpenEvent*>(e)->file();
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
