/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_FORMAT_READER_H
#define FOCUSWRITER_FORMAT_READER_H

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

private:
	virtual void readData(QIODevice* device) = 0;
};

#endif // FOCUSWRITER_FORMAT_READER_H
