/*
	SPDX-FileCopyrightText: 2013-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DOCX_WRITER_H
#define FOCUSWRITER_DOCX_WRITER_H

#include <QCoreApplication>
#include <QString>
#include <QXmlStreamWriter>
class QIODevice;
class QTextBlock;
class QTextBlockFormat;
class QTextCharFormat;
class QTextDocument;

class DocxWriter
{
	Q_DECLARE_TR_FUNCTIONS(DocxWriter)

public:
	explicit DocxWriter();

	QString errorString() const
	{
		return m_error;
	}

	void setStrict(bool strict);
	bool write(QIODevice* device, const QTextDocument* document);

private:
	QByteArray writeDocument(const QTextDocument* document);
	void writeParagraph(const QTextBlock& block);
	void writeText(const QString& text, int start, int end);
	void writeParagraphProperties(const QTextBlockFormat& block_format, const QTextCharFormat& char_format);
	bool writeRunProperties(const QTextCharFormat& char_format, const QString& parent_element = QString());
	void writePropertyElement(const QString& element, bool& empty);
	void writePropertyElement(const QString& element, const QString& parent_element, bool& empty);

private:
	QXmlStreamWriter m_xml;
	bool m_strict;
	QString m_error;
};

#endif // FOCUSWRITER_DOCX_WRITER_H
