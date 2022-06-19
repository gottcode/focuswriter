/*
	SPDX-FileCopyrightText: 2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_WORD_REF
#define FOCUSWRITER_WORD_REF

class WordRef
{
public:
	explicit WordRef(int position = 0, int length = 0) :
		m_position(position),
		m_length(length)
	{
	}

	bool isNull() const
	{
		return !m_length;
	}

	int length() const
	{
		return m_length;
	}

	int position() const
	{
		return m_position;
	}

private:
	int m_position;
	int m_length;
};

#endif // FOCUSWRITER_WORD_REF
