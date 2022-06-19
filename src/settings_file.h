/*
	SPDX-FileCopyrightText: 2010-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SETTINGS_FILE_H
#define FOCUSWRITER_SETTINGS_FILE_H

class SettingsFile
{
public:
	explicit SettingsFile()
		: m_changed(false)
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

#endif // FOCUSWRITER_SETTINGS_FILE_H
