/***********************************************************************
 *
 * Copyright (C) 2010, 2012, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "session.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QUuid>

//-----------------------------------------------------------------------------

QString Session::m_path;

//-----------------------------------------------------------------------------

Session::Session(const QString& id) :
	m_id(id),
	m_default(id.isEmpty())
{
	QString path = pathFromId(m_id);
	if (!m_id.isEmpty() && QFile::exists(path)) {
		m_data = new QSettings(path, QSettings::IniFormat);
		m_name = m_data->value("Name").toString();
	} else {
		m_data = new QSettings;
		m_name = tr("Default");
		m_default = true;
	}
}

//-----------------------------------------------------------------------------

Session::~Session()
{
	delete m_data;
	m_data = 0;
}

//-----------------------------------------------------------------------------

int Session::active() const
{
	return m_data->value("Save/Active").toInt();
}

//-----------------------------------------------------------------------------

QSettings* Session::data() const
{
	return m_data;
}

//-----------------------------------------------------------------------------

QStringList Session::files() const
{
	return m_data->value("Save/Current").toStringList();
}

//-----------------------------------------------------------------------------

QString Session::id() const
{
	return m_id;
}

//-----------------------------------------------------------------------------

QString Session::name() const
{
	return m_name;
}

//-----------------------------------------------------------------------------

QStringList Session::positions() const
{
	return m_data->value("Save/Positions").toStringList();
}

//-----------------------------------------------------------------------------

QString Session::theme() const
{
	return m_data->value("ThemeManager/Theme").toString();
}

//-----------------------------------------------------------------------------

bool Session::themeDefault() const
{
	return m_data->value("ThemeManager/ThemeDefault", false).toBool();
}

//-----------------------------------------------------------------------------

void Session::setName(const QString& name)
{
	if (m_default) {
		return;
	}

	m_name = name;
	m_data->setValue("Name", m_name);
}

//-----------------------------------------------------------------------------

void Session::setTheme(const QString& theme, bool is_default)
{
	m_data->setValue("ThemeManager/Theme", theme);
	m_data->setValue("ThemeManager/ThemeDefault", is_default);
}

//-----------------------------------------------------------------------------

QString Session::createId()
{
	return QUuid::createUuid().toString().mid(1, 36);
}

//-----------------------------------------------------------------------------

bool Session::exists(const QString& name)
{
	QDir dir(m_path, "*.session");
	QStringList sessions = dir.entryList(QDir::Files);
	for (const QString& id : sessions) {
		QSettings session(dir.filePath(id), QSettings::IniFormat);
		if (session.value("Name").toString() == name) {
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

QString Session::path()
{
	return m_path;
}

//-----------------------------------------------------------------------------

QString Session::pathFromId(const QString& id)
{
	return m_path + "/" + id + ".session";
}

//-----------------------------------------------------------------------------

void Session::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------
