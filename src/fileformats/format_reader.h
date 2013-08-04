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

#ifndef FORMAT_READER_H
#define FORMAT_READER_H

#include <QString>
#include <QTextCursor>
class QIODevice;
class QTextDocument;

class FormatReader
{
public:
	virtual ~FormatReader()
	{
	}

	QByteArray encoding() const
	{
		return m_encoding;
	}

	QString errorString() const
	{
		return m_error;
	}

	bool hasError() const
	{
		return !m_error.isEmpty();
	}

	void read(QIODevice* device, QTextDocument* document)
	{
		m_cursor = QTextCursor(document);
		readData(device);
	}

	void read(QIODevice* device, const QTextCursor& cursor)
	{
		m_cursor = cursor;
		readData(device);
	}

	enum { Type = 0 };
	virtual int type() const
	{
		return Type;
	}

protected:
	QTextCursor m_cursor;
	QString m_error;
	QByteArray m_encoding;

private:
	virtual void readData(QIODevice* device) = 0;
};

#endif
