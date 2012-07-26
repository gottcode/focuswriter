/***********************************************************************
 *
 * Copyright (C) 2012 Graeme Gott <graeme@gottcode.org>
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

	QByteArray codePage() const;

	void setCodePage(const QByteArray& codepage);
	void setDocument(QTextDocument* document);
	void setFileName(const QString& filename);
	void setType(const QString& type);

	bool write();

private:
	QString m_filename;
	QString m_type;
	QByteArray m_codepage;
	QTextDocument* m_document;
};

inline QByteArray DocumentWriter::codePage() const
{
	return m_codepage;
}

inline void DocumentWriter::setCodePage(const QByteArray& codepage)
{
	m_codepage = codepage;
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

#endif
