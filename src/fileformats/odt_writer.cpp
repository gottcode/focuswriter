/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2015 Graeme Gott <graeme@gottcode.org>
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

#include "odt_writer.h"

#include <QBuffer>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextFragment>
#include <QtZipWriter>

//-----------------------------------------------------------------------------

OdtWriter::OdtWriter() :
	m_flat(false)
{
}

//-----------------------------------------------------------------------------

void OdtWriter::setFlatXML(bool flat)
{
	m_flat = flat;
}

//-----------------------------------------------------------------------------

bool OdtWriter::write(QIODevice* device, const QTextDocument* document)
{
	if (!m_flat) {
		return writeCompressed(device, document);
	} else {
		return writeUncompressed(device, document);
	}
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeCompressed(QIODevice* device, const QTextDocument* document)
{
	QtZipWriter zip(device);
	if (zip.status() != QtZipWriter::NoError) {
		return false;
	}

	zip.setCompressionPolicy(QtZipWriter::NeverCompress);
	zip.addFile(QString::fromLatin1("mimetype"),
		"application/vnd.oasis.opendocument.text");
	zip.setCompressionPolicy(QtZipWriter::AlwaysCompress);

	zip.addFile(QString::fromLatin1("META-INF/manifest.xml"),
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" manifest:version=\"1.2\">\n"
		" <manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.2\" manifest:media-type=\"application/vnd.oasis.opendocument.text\"/>\n"
		" <manifest:file-entry manifest:full-path=\"content.xml\" manifest:media-type=\"text/xml\"/>\n"
		" <manifest:file-entry manifest:full-path=\"styles.xml\" manifest:media-type=\"text/xml\"/>\n"
		"</manifest:manifest>\n");

	zip.addFile(QString::fromLatin1("content.xml"),
		writeDocument(document));

	zip.addFile(QString::fromLatin1("styles.xml"),
		writeStylesDocument(document));

	zip.close();

	return zip.status() == QtZipWriter::NoError;
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeUncompressed(QIODevice* device, const QTextDocument* document)
{
	m_xml.setDevice(device);
	m_xml.setCodec("UTF-8");
	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:office:1.0"), QString::fromLatin1("office"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:style:1.0"), QString::fromLatin1("style"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:text:1.0"), QString::fromLatin1("text"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"), QString::fromLatin1("fo"));

	m_xml.writeStartDocument();
	m_xml.writeStartElement(QString::fromLatin1("office:document"));
	m_xml.writeAttribute(QString::fromLatin1("office:mimetype"), QString::fromLatin1("application/vnd.oasis.opendocument.text"));
	m_xml.writeAttribute(QString::fromLatin1("office:version"), QString::fromLatin1("1.2"));

	writeStyles(document);
	writeAutomaticStyles(document);
	writeBody(document);

	m_xml.writeEndElement();
	m_xml.writeEndDocument();

	return !m_xml.hasError();
}

//-----------------------------------------------------------------------------

QByteArray OdtWriter::writeDocument(const QTextDocument* document)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	m_xml.setDevice(&buffer);
	m_xml.setCodec("UTF-8");
	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:office:1.0"), QString::fromLatin1("office"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:style:1.0"), QString::fromLatin1("style"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:text:1.0"), QString::fromLatin1("text"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"), QString::fromLatin1("fo"));

	m_xml.writeStartDocument();
	m_xml.writeStartElement(QString::fromLatin1("office:document-content"));
	m_xml.writeAttribute(QString::fromLatin1("office:version"), QString::fromLatin1("1.2"));

	writeAutomaticStyles(document);
	writeBody(document);

	m_xml.writeEndElement();
	m_xml.writeEndDocument();

	buffer.close();
	return data;
}

//-----------------------------------------------------------------------------

QByteArray OdtWriter::writeStylesDocument(const QTextDocument* document)
{
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::WriteOnly);

	m_xml.setDevice(&buffer);
	m_xml.setCodec("UTF-8");
	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:office:1.0"), QString::fromLatin1("office"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:style:1.0"), QString::fromLatin1("style"));
	m_xml.writeNamespace(QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"), QString::fromLatin1("fo"));

	m_xml.writeStartDocument();
	m_xml.writeStartElement(QString::fromLatin1("office:document-styles"));
	m_xml.writeAttribute(QString::fromLatin1("office:version"), QString::fromLatin1("1.2"));

	writeStyles(document);

	m_xml.writeEndElement();
	m_xml.writeEndDocument();

	buffer.close();
	return data;
}

//-----------------------------------------------------------------------------

void OdtWriter::writeStyles(const QTextDocument*)
{
	static const std::vector<std::vector<QString>> styles = {
		{"Normal", "Normal", "0", "12pt", "normal"},
		{"Heading-1", "Heading 1", "1", "18pt", "bold"},
		{"Heading-2", "Heading 2", "2", "16pt", "bold"},
		{"Heading-3", "Heading 3", "3", "14pt", "bold"},
		{"Heading-4", "Heading 4", "4", "12pt", "bold"},
		{"Heading-5", "Heading 5", "5", "10pt", "bold"},
		{"Heading-6", "Heading 6", "6", "8pt", "bold"}
	};

	m_xml.writeStartElement("office:styles");
	for (const auto& style : styles) {
		m_xml.writeStartElement("style:style");
		m_xml.writeAttribute("style:name", style[0]);
		m_xml.writeAttribute("style:display-name", style[1]);
		m_xml.writeAttribute("style:family", "paragraph");
		if (style[0] != "Normal") {
			m_xml.writeAttribute("style:parent-style-name", "Normal");
			m_xml.writeAttribute("style:next-style-name", "Normal");
			m_xml.writeAttribute("style:default-outline-level", style[2]);
		}

		m_xml.writeStartElement("style:text-properties");
		m_xml.writeAttribute("fo:font-size", style[3]);
		m_xml.writeAttribute("fo:font-weight", style[4]);
		m_xml.writeEndElement();

		m_xml.writeEndElement();
	}
	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void OdtWriter::writeAutomaticStyles(const QTextDocument* document)
{
	m_xml.writeStartElement(QString::fromLatin1("office:automatic-styles"));

	QVector<QTextFormat> formats = document->allFormats();

	// Find all used styles
	QVector<int> text_styles;
	QVector<int> paragraph_styles;
	int index = 0;
	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		index = block.blockFormatIndex();
		if (!paragraph_styles.contains(index)) {
			int heading = block.blockFormat().property(QTextFormat::UserProperty).toInt();
			if (!heading) {
				paragraph_styles.append(index);
			} else {
				m_styles.insert(index, QString("Heading-%1").arg(heading));
			}
		}
		for (QTextBlock::iterator iter = block.begin(); !(iter.atEnd()); ++iter) {
			index = iter.fragment().charFormatIndex();
			if (!text_styles.contains(index) && formats.at(index).propertyCount()) {
				text_styles.append(index);
			}
		}
	}

	// Write text styles
	int text_style = 1;
	for (int i = 0; i < text_styles.size(); ++i) {
		int index = text_styles.at(i);
		const QTextFormat& format = formats.at(index);
		QString name = QString::fromLatin1("T") + QString::number(text_style);
		if (writeTextStyle(format.toCharFormat(), name)) {
			m_styles.insert(index, name);
			++text_style;
		}
	}

	// Write paragraph styles
	int paragraph_style = 1;
	for (int i = 0; i < paragraph_styles.size(); ++i) {
		int index = paragraph_styles.at(i);
		const QTextFormat& format = formats.at(index);
		QString name = QString::fromLatin1("P") + QString::number(paragraph_style);
		if (writeParagraphStyle(format.toBlockFormat(), name)) {
			m_styles.insert(index, name);
			++paragraph_style;
		} else {
			m_styles.insert(index, "Normal");
		}
	}

	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeParagraphStyle(const QTextBlockFormat& format, const QString& name)
{
	QXmlStreamAttributes attributes;
	bool rtl = format.layoutDirection() == Qt::RightToLeft;
	if (rtl) {
		attributes.append(QString::fromLatin1("style:writing-mode"), QString::fromLatin1("rl"));
	}

	Qt::Alignment align = format.alignment();
	if (rtl && (align & Qt::AlignLeft)) {
		attributes.append(QString::fromLatin1("fo:text-align"), QString::fromLatin1("left"));
	} else if (align & Qt::AlignRight) {
		attributes.append(QString::fromLatin1("fo:text-align"), QString::fromLatin1("right"));
	} else if (align & Qt::AlignCenter) {
		attributes.append(QString::fromLatin1("fo:text-align"), QString::fromLatin1("center"));
	} else if (align & Qt::AlignJustify) {
		attributes.append(QString::fromLatin1("fo:text-align"), QString::fromLatin1("justify"));
	}

	if (format.indent() > 0) {
		attributes.append(QString::fromLatin1("fo:margin-left"), QString::number(format.indent() * 0.5) + QString::fromLatin1("in"));
	}

	if (attributes.isEmpty()) {
		return false;
	}

	m_xml.writeStartElement(QString::fromLatin1("style:style"));
	m_xml.writeAttribute(QString::fromLatin1("style:name"), name);
	m_xml.writeAttribute(QString::fromLatin1("style:family"), QString::fromLatin1("paragraph"));
	m_xml.writeAttribute(QString::fromLatin1("style:parent-style-name"), QString::fromLatin1("Normal"));

	m_xml.writeEmptyElement(QString::fromLatin1("style:paragraph-properties"));
	m_xml.writeAttributes(attributes);
	m_xml.writeEndElement();

	return true;
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeTextStyle(const QTextCharFormat& format, const QString& name)
{
	QXmlStreamAttributes attributes;
	if (format.fontWeight() == QFont::Bold) {
		attributes.append(QString::fromLatin1("fo:font-weight"), QString::fromLatin1("bold"));
	}
	if (format.fontItalic()) {
		attributes.append(QString::fromLatin1("fo:font-style"), QString::fromLatin1("italic"));
	}
	if (format.fontUnderline()) {
		attributes.append(QString::fromLatin1("style:text-underline-type"), QString::fromLatin1("single"));
		attributes.append(QString::fromLatin1("style:text-underline-style"), QString::fromLatin1("solid"));
	}
	if (format.fontStrikeOut()) {
		attributes.append(QString::fromLatin1("style:text-line-through-type"), QString::fromLatin1("single"));
	}
	if (format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
		attributes.append(QString::fromLatin1("style:text-position"), QString::fromLatin1("super"));
	} else if (format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
		attributes.append(QString::fromLatin1("style:text-position"), QString::fromLatin1("sub"));
	}

	if (attributes.isEmpty()) {
		return false;
	}

	m_xml.writeStartElement(QString::fromLatin1("style:style"));
	m_xml.writeAttribute(QString::fromLatin1("style:name"), name);
	m_xml.writeAttribute(QString::fromLatin1("style:family"), QString::fromLatin1("text"));

	m_xml.writeEmptyElement(QString::fromLatin1("style:text-properties"));
	m_xml.writeAttributes(attributes);
	m_xml.writeEndElement();

	return true;
}

//-----------------------------------------------------------------------------

void OdtWriter::writeBody(const QTextDocument* document)
{
	m_xml.writeStartElement(QString::fromLatin1("office:body"));
	m_xml.writeStartElement(QString::fromLatin1("office:text"));

	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		int heading = block.blockFormat().property(QTextFormat::UserProperty).toInt();
		if (!heading) {
			m_xml.writeStartElement(QString::fromLatin1("text:p"));
		} else {
			m_xml.writeStartElement(QString::fromLatin1("text:h"));
			m_xml.writeAttribute(QString::fromLatin1("text:outline-level"), QString::number(heading));
		}
		m_xml.writeAttribute(QString::fromLatin1("text:style-name"), m_styles.value(block.blockFormatIndex()));
		m_xml.setAutoFormatting(false);

		for (QTextBlock::iterator iter = block.begin(); !(iter.atEnd()); ++iter) {
			QTextFragment fragment = iter.fragment();
			QString style = m_styles.value(fragment.charFormatIndex());
			if (!style.isEmpty()) {
				m_xml.writeStartElement(QString::fromLatin1("text:span"));
				m_xml.writeAttribute(QString::fromLatin1("text:style-name"), style);
			}

			QString text = fragment.text();
			int start = 0;
			int spaces = -1;
			for (int i = 0; i < text.length(); ++i) {
				QChar c = text.at(i);
				if (c.unicode() == 0x0) {
					m_xml.writeCharacters(text.mid(start, i - start));
					start = i + 1;
				} else if (c.unicode() == 0x0009) {
					m_xml.writeCharacters(text.mid(start, i - start));
					m_xml.writeEmptyElement(QString::fromLatin1("text:tab"));
					start = i + 1;
				} else if (c.unicode() == 0x2028) {
					m_xml.writeCharacters(text.mid(start, i - start));
					m_xml.writeEmptyElement(QString::fromLatin1("text:line-break"));
					start = i + 1;
				} else if (c.unicode() == 0x0020) {
					++spaces;
				} else if (spaces > 0) {
					m_xml.writeCharacters(text.mid(start, i - spaces - start));
					m_xml.writeEmptyElement(QString::fromLatin1("text:s"));
					m_xml.writeAttribute(QString::fromLatin1("text:c"), QString::number(spaces));
					spaces = -1;
					start = i;
				} else {
					spaces = -1;
				}
			}
			if (spaces > 0) {
				m_xml.writeCharacters(text.mid(start, text.length() - spaces - start));
				m_xml.writeEmptyElement(QString::fromLatin1("text:s"));
				m_xml.writeAttribute(QString::fromLatin1("text:c"), QString::number(spaces));
			} else {
				m_xml.writeCharacters(text.mid(start));
			}

			if (!style.isEmpty()) {
				m_xml.writeEndElement();
			}
		}

		m_xml.writeEndElement();
		m_xml.setAutoFormatting(true);
	}

	m_xml.writeEndElement();
	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------
