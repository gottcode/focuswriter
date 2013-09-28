/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef RANGED_STRING_H
#define RANGED_STRING_H

#include <QString>
#include <QStringList>
#include <QVariant>

class RangedString
{
public:
	RangedString(const QStringList& allowed) :
		m_value(!allowed.isEmpty() ? allowed.first() : ""), m_allowed(allowed)
	{
	}

	RangedString& operator=(const QString& value)
	{
		m_value = m_allowed.contains(value) ? value : m_value;
		return *this;
	}

	QStringList allowedValues() const
	{
		return m_allowed;
	}

	QString value() const
	{
		return m_value;
	}

	operator QString() const
	{
		return m_value;
	}

	bool operator!=(const QString& string) const
	{
		return m_value != string;
	}

private:
	QString m_value;
	const QStringList m_allowed;
};

#endif
