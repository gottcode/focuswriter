/***********************************************************************
 *
 * Copyright (C) 2010, 2013 Graeme Gott <graeme@gottcode.org>
 *
 * Derived from KWord's rtfimport_tokenizer.cpp
 *  Copyright (C) 2001 Ewald Snel <ewald@rambo.its.tudelft.nl>
 *  Copyright (C) 2001 Tomasz Grobelny <grotk@poczta.onet.pl>
 *  Copyright (C) 2005 Tommi Rantala <tommi.rantala@cs.helsinki.fi>
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

#include "rtf_tokenizer.h"

#include <QCoreApplication>
#include <QIODevice>

//-----------------------------------------------------------------------------

RtfTokenizer::RtfTokenizer() :
	m_device(0),
	m_position(0),
	m_value(0),
	m_has_value(false)
{
	m_buffer.reserve(8192);
	m_text.reserve(8192);
}

//-----------------------------------------------------------------------------

bool RtfTokenizer::hasNext() const
{
	return (m_position < m_buffer.size() - 1) || !m_device->atEnd();
}

//-----------------------------------------------------------------------------

void RtfTokenizer::readNext()
{
	// Reset values
	m_type = TextToken;
	m_hex.clear();
	m_text.resize(0);
	m_value = 0;
	m_has_value = false;
	if (!m_device) {
		return;
	}

	// Read first character
	char c;
	do {
		c = next();
	} while (c == '\n' || c == '\r');

	// Determine token type
	if (c == '{') {
		m_type = StartGroupToken;
	} else if (c == '}') {
		m_type = EndGroupToken;
	} else if (c == '\\') {
		m_type = ControlWordToken;

		c = next();

		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
			// Read control word
			while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				m_text.append(c);
				c = next();
			}

			// Read integer value
			int sign = (c != '-') ? 1 : -1;
			if (sign == -1) {
				c = next();
			}
			QByteArray value;
			while (isdigit(c)) {
				value.append(c);
				c = next();
			}
			m_has_value = !value.isEmpty();
			m_value = value.toInt() * sign;

			// Eat space after control word
			if (c != ' ') {
				--m_position;
			}

			// Eat binary value
			if (m_text == "bin") {
				if (m_value > 0) {
					for (int i = 0; i < m_value; i++) {
						c = next();
					}
				}
				return readNext();
			}
		} else if (c == '\'') {
			// Read hexadecimal value
			m_text.append(c);
			QByteArray hex(2, 0);
			hex[0] = next();
			hex[1] = next();
			m_hex.append(hex.toInt(0, 16));
		} else {
			// Read escaped character
			m_text.append(c);
		}
	} else {
		// Read text
		m_type = TextToken;
		while (c != '\\' && c != '{' && c != '}' && c != '\n' && c != '\r') {
			m_text.append(c);
			c = next();
		}
		m_position--;
	}
}

//-----------------------------------------------------------------------------

void RtfTokenizer::setDevice(QIODevice* device)
{
	m_device = device;
}

//-----------------------------------------------------------------------------

char RtfTokenizer::next()
{
	m_position++;
	if (m_position >= m_buffer.size()) {
		m_buffer.resize(8192);
		int size = m_device->read(m_buffer.data(), m_buffer.size());
		if (size < 1) {
			throw tr("Unexpectedly reached end of file.");
		}
		m_buffer.resize(size);
		m_position = 0;
		QCoreApplication::processEvents();
	}
	return m_buffer.at(m_position);
}

//-----------------------------------------------------------------------------
