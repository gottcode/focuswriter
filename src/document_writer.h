/*
	SPDX-FileCopyrightText: 2012-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DOCUMENT_WRITER_H
#define FOCUSWRITER_DOCUMENT_WRITER_H

#include <QString>
class QTextDocument;

class DocumentWriter
{
public:
	explicit DocumentWriter();
	~DocumentWriter();

	void setDocument(const QTextDocument* document);
	void setFileName(const QString& filename);
	void setType(const QString& type);
	void setWriteByteOrderMark(bool write_bom);

	bool write();

private:
	QString m_filename;
	QString m_type;
	const QTextDocument* m_document;
	bool m_write_bom;
};

inline void DocumentWriter::setDocument(const QTextDocument* document)
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

#endif // FOCUSWRITER_DOCUMENT_WRITER_H
