/*
	SPDX-FileCopyrightText: 2010-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "session.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QUuid>

//-----------------------------------------------------------------------------

QString Session::m_path;

//-----------------------------------------------------------------------------

Session::Session(const QString& id)
	: m_id(id)
	, m_default(id.isEmpty())
{
	const QString path = pathFromId(m_id);
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
	const QDir dir(m_path, "*.session");
	const QStringList sessions = dir.entryList(QDir::Files);
	for (const QString& id : sessions) {
		const QSettings session(dir.filePath(id), QSettings::IniFormat);
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
