/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef RTF_WRITER_H
#define RTF_WRITER_H

#include <QByteArray>
#include <QString>
class QIODevice;
class QTextCodec;
class QTextDocument;

class RtfWriter
{
public:
	RtfWriter(const QByteArray& encoding = QByteArray());

	QByteArray encoding() const;

	bool write(QIODevice* device, const QTextDocument* text);

private:
	void setCodec(QTextCodec* codec);
	QByteArray fromUnicode(const QString& string) const;

private:
	QByteArray m_encoding;
	QTextCodec* m_codec;
	bool m_supports_ascii;
	QByteArray m_header;
};

inline QByteArray RtfWriter::encoding() const
{
	return m_encoding;
}

#endif
