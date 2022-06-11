/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_RANGED_STRING_H
#define FOCUSWRITER_RANGED_STRING_H

#include <QString>
#include <QStringList>
#include <QVariant>

class RangedString
{
public:
	explicit RangedString(const QStringList& allowed)
		: m_value(!allowed.isEmpty() ? allowed.first() : QString())
		, m_allowed(allowed)
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

#endif // FOCUSWRITER_RANGED_STRING_H
