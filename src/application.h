/*
	SPDX-FileCopyrightText: 2011-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_APPLICATION_H
#define FOCUSWRITER_APPLICATION_H

class Window;

#include <QStringList>
#include <QtSingleApplication>

class Application : public QtSingleApplication
{
public:
	Application(int& argc, char** argv);

	QStringList files() const
	{
		return m_files;
	}

	bool createWindow(const QStringList& files);

protected:
	bool event(QEvent* e) override;

private:
	QStringList m_files;
	Window* m_window;
};

#endif // FOCUSWRITER_APPLICATION_H
