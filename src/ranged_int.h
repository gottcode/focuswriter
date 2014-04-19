/***********************************************************************
 *
 * Copyright (C) 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#ifndef RANGED_INT_H
#define RANGED_INT_H

#include <QVariant>

class RangedInt
{
public:
	RangedInt(int min, int max) :
		m_value(min), m_min(min), m_max(max)
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

#endif
