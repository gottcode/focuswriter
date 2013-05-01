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

#include "docx_writer.h"

#include "zip_writer.h"

#include <QBuffer>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>

//-----------------------------------------------------------------------------

DocxWriter::DocxWriter() :
	m_strict(false)
{
}

//-----------------------------------------------------------------------------

void DocxWriter::setStrict(bool strict)
{
	m_strict = strict;
}

//-----------------------------------------------------------------------------

bool DocxWriter::write(const QString& filename, QTextDocument* document)
{
	ZipWriter zip(filename);

	if (!zip.addFile(QString::fromLatin1("_rels/.rels"),
			"<?xml version=\"1.0\"?>"
			"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
			"<Relationship Target=\"word/document.xml\" Id=\"pkgRId0\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\"/>"
			"</Relationships>")) {
		return false;
	}

	if (!zip.addFile(QString::fromLatin1("word/_rels/document.xml.rels"),
			"<?xml version=\"1.0\"?>"
			"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
			"<Relationship Target=\"styles.xml\" Id=\"docRId0\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\"/>"
			"</Relationships>")) {
		return false;
	}

	if (!zip.addFile(QString::fromLatin1("word/document.xml"),
			writeDocument(document))) {
		return false;
	}

	if (!zip.addFile(QString::fromLatin1("word/styles.xml"),
			"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
			"<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"/>")) {
		return false;
	}

	if (!zip.addFile(QString::fromLatin1("[Content_Types].xml"),
			"<?xml version=\"1.0\"?>"
			"<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
			"<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
			"<Default Extension=\"xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
			"<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
			"</Types>")) {
		return false;
	}

	return zip.close();
}

//-----------------------------------------------------------------------------

QByteArray DocxWriter::writeDocument(QTextDocument* document)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);
	m_xml.setDevice(&buffer);
	m_xml.setCodec("UTF-8");
	m_xml.writeNamespace(QString::fromLatin1("http://schemas.openxmlformats.org/wordprocessingml/2006/main"), QString::fromLatin1("w"));
	m_xml.writeStartDocument(QString::fromLatin1("1.0"), true);

	m_xml.writeStartElement(QString::fromLatin1("w:document"));
	m_xml.writeStartElement(QString::fromLatin1("w:body"));

	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		writeParagraph(block);
	}

	m_xml.writeEndElement();
	m_xml.writeEndElement();

	m_xml.writeEndDocument();
	buffer.close();

	return data;
}

//-----------------------------------------------------------------------------

void DocxWriter::writeParagraph(const QTextBlock& block)
{
	m_xml.writeStartElement(QString::fromLatin1("w:p"));
	writeParagraphProperties(block.blockFormat(), block.charFormat());

	for (QTextBlock::iterator iter = block.begin(); !(iter.atEnd()); ++iter) {
		m_xml.writeStartElement(QString::fromLatin1("w:r"));

		QTextFragment fragment = iter.fragment();
		writeRunProperties(fragment.charFormat());

		QString text = fragment.text();
		int start = 0;
		int count = text.length();
		for (int i = 0; i < count; ++i) {
			QChar c = text.at(i);
			if (c.unicode() == 0x0009) {
				writeText(text, start, i);
				m_xml.writeEmptyElement(QString::fromLatin1("w:tab"));
				start = i + 1;
			} else if (c.unicode() == 0x2028) {
				writeText(text, start, i);
				m_xml.writeEmptyElement(QString::fromLatin1("w:br"));
				start = i + 1;
			}
		}
		writeText(text, start, count);

		m_xml.writeEndElement();
	}

	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void DocxWriter::writeText(const QString& text, int start, int end)
{
	if (start < end) {
		m_xml.writeStartElement(QString::fromLatin1("w:t"));
		m_xml.writeAttribute(QString::fromLatin1("xml:space"), QString::fromLatin1("preserve"));
		m_xml.writeCharacters(text.mid(start, end - start));
		m_xml.writeEndElement();
	}
}

//-----------------------------------------------------------------------------

void DocxWriter::writeParagraphProperties(const QTextBlockFormat& block_format, const QTextCharFormat& char_format)
{
	bool empty = true;

	bool rtl = block_format.layoutDirection() == Qt::RightToLeft;
	if (rtl) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:textDirection"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("rl"));
	}

	Qt::Alignment align = block_format.alignment();
	if (rtl && (align & Qt::AlignLeft)) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:jc"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), m_strict ? QString::fromLatin1("start") : QString::fromLatin1("left"));
	} else if (align & Qt::AlignRight) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:jc"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), m_strict ? QString::fromLatin1("end") : QString::fromLatin1("right"));
	} else if (align & Qt::AlignCenter) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:jc"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("center"));
	} else if (align & Qt::AlignJustify) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:jc"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("both"));
	}

	if (block_format.indent() > 0) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:ind"));
		QString indent = QString::number(block_format.indent() * 15);
		if (m_strict) {
			m_xml.writeAttribute(QString::fromLatin1("w:start"), indent);
		} else if (rtl) {
			m_xml.writeAttribute(QString::fromLatin1("w:right"), indent);
		} else {
			m_xml.writeAttribute(QString::fromLatin1("w:left"), indent);
		}
	}

	empty &= writeRunProperties(char_format, QString::fromLatin1("w:pPr"));

	if (!empty) {
		m_xml.writeEndElement();
	}
}

//-----------------------------------------------------------------------------

bool DocxWriter::writeRunProperties(const QTextCharFormat& char_format, const QString& parent_element)
{
	bool empty = true;

	if (char_format.fontWeight() == QFont::Bold) {
		writePropertyElement(QString::fromLatin1("w:rPr"), parent_element, empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:b"));
	}

	if (char_format.fontItalic()) {
		writePropertyElement(QString::fromLatin1("w:rPr"), parent_element, empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:i"));
	}

	if (char_format.fontUnderline()) {
		writePropertyElement(QString::fromLatin1("w:rPr"), parent_element, empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:u"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("single"));
	}

	if (char_format.fontStrikeOut()) {
		writePropertyElement(QString::fromLatin1("w:rPr"), parent_element, empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:strike"));
	}

	if (char_format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
		writePropertyElement(QString::fromLatin1("w:rPr"), parent_element, empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:vertAlign"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("superscript"));
	} else if (char_format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
		writePropertyElement(QString::fromLatin1("w:rPr"), parent_element, empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:vertAlign"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("subscript"));
	}

	if (!empty) {
		m_xml.writeEndElement();
	}
	return empty;
}

//-----------------------------------------------------------------------------

void DocxWriter::writePropertyElement(const QString& element, bool& empty)
{
	if (empty) {
		empty = false;
		m_xml.writeStartElement(element);
	}
}

//-----------------------------------------------------------------------------

void DocxWriter::writePropertyElement(const QString& element, const QString& parent_element, bool& empty)
{
	if (empty) {
		empty = false;
		if (!parent_element.isEmpty()) {
			m_xml.writeStartElement(parent_element);
		}
		m_xml.writeStartElement(element);
	}
}

//-----------------------------------------------------------------------------
