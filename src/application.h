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

#ifndef APPLICATION_H
#define APPLICATION_H

class Window;

#include <QStringList>
#include <QtSingleApplication>

class Application : public QtSingleApplication
{
public:
	Application(int& argc, char** argv);

	QStringList files() const
		{ return m_files; }

	bool createWindow();

protected:
	virtual bool event(QEvent* e);

private:
	QStringList m_files;
	Window* m_window;
};

#endif
