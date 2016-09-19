/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2016 Graeme Gott <graeme@gottcode.org>
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

#include <QBuffer>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QtZipWriter>

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

bool DocxWriter::write(QIODevice* device, const QTextDocument* document)
{
	QtZipWriter zip(device);
	if (zip.status() != QtZipWriter::NoError) {
		return false;
	}

	zip.addFile(QString::fromLatin1("_rels/.rels"),
		"<?xml version=\"1.0\"?>"
		"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
		"<Relationship Target=\"word/document.xml\" Id=\"pkgRId0\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\"/>"
		"</Relationships>");

	zip.addFile(QString::fromLatin1("word/_rels/document.xml.rels"),
		"<?xml version=\"1.0\"?>"
		"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
		"<Relationship Target=\"styles.xml\" Id=\"docRId0\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\"/>"
		"</Relationships>");

	zip.addFile(QString::fromLatin1("word/document.xml"),
		writeDocument(document));

	zip.addFile(QString::fromLatin1("word/styles.xml"),
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
		"<w:styles xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
		"<w:style w:type=\"paragraph\" w:styleId=\"Normal\">"
			"<w:name w:val=\"Normal\"/>"
			"<w:pPr/>"
			"<w:rPr>"
				"<w:sz w:val=\"24\"/>"
			"</w:rPr>"
		"</w:style>"
		"<w:style w:type=\"paragraph\" w:styleId=\"Heading1\">"
			"<w:name w:val=\"Heading 1\"/>"
			"<w:basedOn w:val=\"Normal\"/>"
			"<w:next w:val=\"Normal\"/>"
			"<w:pPr>"
				"<w:outlineLvl w:val=\"0\"/>"
			"</w:pPr>"
			"<w:rPr>"
				"<w:b/>"
				"<w:sz w:val=\"36\"/>"
			"</w:rPr>"
		"</w:style>"
		"<w:style w:type=\"paragraph\" w:styleId=\"Heading2\">"
			"<w:name w:val=\"Heading 2\"/>"
			"<w:basedOn w:val=\"Normal\"/>"
			"<w:next w:val=\"Normal\"/>"
			"<w:pPr>"
				"<w:outlineLvl w:val=\"1\"/>"
			"</w:pPr>"
			"<w:rPr>"
				"<w:b/>"
				"<w:sz w:val=\"32\"/>"
			"</w:rPr>"
		"</w:style>"
		"<w:style w:type=\"paragraph\" w:styleId=\"Heading3\">"
			"<w:name w:val=\"Heading 3\"/>"
			"<w:basedOn w:val=\"Normal\"/>"
			"<w:next w:val=\"Normal\"/>"
			"<w:pPr>"
				"<w:outlineLvl w:val=\"2\"/>"
			"</w:pPr>"
			"<w:rPr>"
				"<w:b/>"
				"<w:sz w:val=\"28\"/>"
			"</w:rPr>"
		"</w:style>"
		"<w:style w:type=\"paragraph\" w:styleId=\"Heading4\">"
			"<w:name w:val=\"Heading 4\"/>"
			"<w:basedOn w:val=\"Normal\"/>"
			"<w:next w:val=\"Normal\"/>"
			"<w:pPr>"
				"<w:outlineLvl w:val=\"3\"/>"
			"</w:pPr>"
			"<w:rPr>"
				"<w:b/>"
				"<w:sz w:val=\"24\"/>"
			"</w:rPr>"
		"</w:style>"
		"<w:style w:type=\"paragraph\" w:styleId=\"Heading5\">"
			"<w:name w:val=\"Heading 5\"/>"
			"<w:basedOn w:val=\"Normal\"/>"
			"<w:next w:val=\"Normal\"/>"
			"<w:pPr>"
				"<w:outlineLvl w:val=\"4\"/>"
			"</w:pPr>"
			"<w:rPr>"
				"<w:b/>"
				"<w:sz w:val=\"20\"/>"
			"</w:rPr>"
		"</w:style>"
		"<w:style w:type=\"paragraph\" w:styleId=\"Heading6\">"
			"<w:name w:val=\"Heading 6\"/>"
			"<w:basedOn w:val=\"Normal\"/>"
			"<w:next w:val=\"Normal\"/>"
			"<w:pPr>"
				"<w:outlineLvl w:val=\"5\"/>"
			"</w:pPr>"
			"<w:rPr>"
				"<w:b/>"
				"<w:sz w:val=\"16\"/>"
			"</w:rPr>"
		"</w:style>"
		"</w:styles>");

	zip.addFile(QString::fromLatin1("[Content_Types].xml"),
		"<?xml version=\"1.0\"?>"
		"<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
		"<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
		"<Default Extension=\"xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
		"<Override PartName=\"/word/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml\"/>"
		"</Types>");

	zip.close();

	return zip.status() == QtZipWriter::NoError;
}

//-----------------------------------------------------------------------------

QByteArray DocxWriter::writeDocument(const QTextDocument* document)
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
			} else if (c.unicode() == 0x0) {
				writeText(text, start, i);
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

	int heading = block_format.property(QTextFormat::UserProperty).toInt();
	if (heading) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:pStyle"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString("Heading%1").arg(heading));
	}

	bool rtl = block_format.layoutDirection() == Qt::RightToLeft;
	if (rtl) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:bidi"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), QString::fromLatin1("1"));
	}

	Qt::Alignment align = block_format.alignment();
	if (rtl && (align & Qt::AlignRight)) {
		writePropertyElement(QString::fromLatin1("w:pPr"), empty);
		m_xml.writeEmptyElement(QString::fromLatin1("w:jc"));
		m_xml.writeAttribute(QString::fromLatin1("w:val"), m_strict ? QString::fromLatin1("start") : QString::fromLatin1("left"));
	} else if ((align & Qt::AlignRight) || (rtl && (align & Qt::AlignLeft))) {
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
		QString indent = QString::number(block_format.indent() * 720);
		if (m_strict) {
			m_xml.writeAttribute(QString::fromLatin1("w:start"), indent);
		} else if (rtl) {
			m_xml.writeAttribute(QString::fromLatin1("w:right"), indent);
		} else {
			m_xml.writeAttribute(QString::fromLatin1("w:left"), indent);
		}
	}

	if (!empty) {
		writeRunProperties(char_format);
	} else {
		empty = writeRunProperties(char_format, QString::fromLatin1("w:pPr"));
	}

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
