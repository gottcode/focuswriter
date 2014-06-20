/***********************************************************************
 *
 * Copyright (C) 2010, 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef SETTINGS_FILE_H
#define SETTINGS_FILE_H

class SettingsFile
{
public:
	SettingsFile() :
		m_changed(false)
	{
	}

	virtual ~SettingsFile()
	{
	}

	bool isChanged() const
	{
		return m_changed;
	}

	void forgetChanges()
	{
		m_changed = false;
		reload();
	}

	void saveChanges()
	{
		if (m_changed) {
			write();
			m_changed = false;
		}
	}

protected:
	template <typename T> void setValue(T& dest, const T& source)
	{
		if (dest != source) {
			dest = source;
			m_changed = true;
		}
	}

	template <typename T1, typename T2> void setValue(T1& dest, const T2& source)
	{
		if (dest != source) {
			dest = source;
			m_changed = true;
		}
	}

private:
	virtual void reload() = 0;
	virtual void write() = 0;

private:
	bool m_changed;
};

#endif
