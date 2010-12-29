/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QUrl>

//-----------------------------------------------------------------------------

QString Session::m_path;

//-----------------------------------------------------------------------------

Session::Session(const QString& name)
	: m_name(name),
	m_default(name == tr("Default"))
{
	QString path = pathFromName(m_name);
	if (QFile::exists(path)) {
		m_data = new QSettings(path, QSettings::IniFormat);
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

void Session::setName(const QString& name)
{
	if (m_default) {
		return;
	}

	QString old_file = m_data->fileName();
	delete m_data;
	m_data = 0;

	m_name = name;
	QString file = pathFromName(m_name);
	QFile::remove(file);
	QFile::rename(old_file, file);

	m_data = new QSettings(file, QSettings::IniFormat);
}

//-----------------------------------------------------------------------------

void Session::setTheme(const QString& theme)
{
	m_data->setValue("ThemeManager/Theme", theme);
}

//-----------------------------------------------------------------------------

QString Session::path()
{
	return m_path;
}

//-----------------------------------------------------------------------------

QString Session::pathFromName(const QString& name)
{
	return m_path + "/" + QUrl::toPercentEncoding(name, " ") + ".session";
}

//-----------------------------------------------------------------------------

QString Session::pathToName(const QString& path)
{
	return QUrl::fromPercentEncoding(QFileInfo(path).baseName().toUtf8());
}

//-----------------------------------------------------------------------------

void Session::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------
