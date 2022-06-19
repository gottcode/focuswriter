/*
	SPDX-FileCopyrightText: 2013-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_RANGED_INT_H
#define FOCUSWRITER_RANGED_INT_H

#include <QVariant>

class RangedInt
{
public:
	RangedInt(int min, int max)
		: m_value(min)
		, m_min(min)
		, m_max(max)
	{
	}

	RangedInt& operator=(int value)
	{
		m_value = (value > m_min) ? ((value < m_max) ? value : m_max) : m_min;
		return *this;
	}

	int minimumValue() const
	{
		return m_min;
	}

	int maximumValue() const
	{
		return m_max;
	}

	int value() const
	{
		return m_value;
	}

	operator int() const
	{
		return m_value;
	}

	bool operator==(int value) const
	{
		return m_value == value;
	}

	bool operator!=(int value) const
	{
		return m_value != value;
	}

private:
	int m_value;
	int m_min;
	int m_max;
};

#endif // FOCUSWRITER_RANGED_INT_H
