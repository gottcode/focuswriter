/***********************************************************************
 *
 * Copyright (C) 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef DOCUMENT_WRITER_H
#define DOCUMENT_WRITER_H

#include <QString>
class QTextDocument;

class DocumentWriter
{
public:
	DocumentWriter();
	~DocumentWriter();

	QByteArray encoding() const;

	void setEncoding(const QByteArray& encoding);
	void setDocument(QTextDocument* document);
	void setFileName(const QString& filename);
	void setType(const QString& type);
	void setWriteByteOrderMark(bool write_bom);

	bool write();

private:
	QString m_filename;
	QString m_type;
	QByteArray m_encoding;
	QTextDocument* m_document;
	bool m_write_bom;
};

inline QByteArray DocumentWriter::encoding() const
{
	return m_encoding;
}

inline void DocumentWriter::setEncoding(const QByteArray& encoding)
{
	m_encoding = encoding;
}

inline void DocumentWriter::setDocument(QTextDocument* document)
{
	m_document = document;
}

inline void DocumentWriter::setFileName(const QString& filename)
{
	m_filename = filename;
}

inline void DocumentWriter::setType(const QString& type)
{
	m_type = type.toLower();
}

inline void DocumentWriter::setWriteByteOrderMark(bool write_bom)
{
	m_write_bom = write_bom;
}

#endif
