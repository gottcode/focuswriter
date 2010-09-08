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

#ifndef RTF_TOKENIZER_H
#define RTF_TOKENIZER_H

#include <QByteArray>
#include <QCoreApplication>
class QIODevice;

namespace RTF
{
	enum TokenType
	{
		StartGroupToken,
		EndGroupToken,
		ControlWordToken,
		TextToken
	};

	class Tokenizer
	{
		Q_DECLARE_TR_FUNCTIONS(Tokenizer)

	public:
		Tokenizer();

		bool hasNext() const;
		bool hasValue() const;
		QByteArray hex() const;
		QByteArray text() const;
		TokenType type() const;
		qint32 value() const;

		void readNext();
		void setDevice(QIODevice* device);

	private:
		char next();

	private:
		QIODevice* m_device;
		QByteArray m_buffer;
		int m_position;

		TokenType m_type;
		QByteArray m_hex;
		QByteArray m_text;
		qint32 m_value;
		bool m_has_value;
	};

	inline bool Tokenizer::hasValue() const
	{
		return m_has_value;
	}

	inline QByteArray Tokenizer::hex() const
	{
		return m_hex;
	}

	inline QByteArray Tokenizer::text() const
	{
		return m_text;
	}

	inline TokenType Tokenizer::type() const
	{
		return m_type;
	}

	inline qint32 Tokenizer::value() const
	{
		return m_value;
	}
}

#endif // TOKENIZER_H
