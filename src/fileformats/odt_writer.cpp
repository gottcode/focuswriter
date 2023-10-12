/*
	SPDX-FileCopyrightText: 2013-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "odt_writer.h"

#include <QBuffer>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextFragment>
#include <QtZipWriter>

//-----------------------------------------------------------------------------

OdtWriter::OdtWriter()
	: m_flat(false)
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
	zip.addFile(QStringLiteral("mimetype"),
		"application/vnd.oasis.opendocument.text");
	zip.setCompressionPolicy(QtZipWriter::AlwaysCompress);

	zip.addFile(QStringLiteral("META-INF/manifest.xml"),
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" manifest:version=\"1.2\">\n"
		" <manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.2\" manifest:media-type=\"application/vnd.oasis.opendocument.text\"/>\n"
		" <manifest:file-entry manifest:full-path=\"content.xml\" manifest:media-type=\"text/xml\"/>\n"
		" <manifest:file-entry manifest:full-path=\"styles.xml\" manifest:media-type=\"text/xml\"/>\n"
		"</manifest:manifest>\n");

	zip.addFile(QStringLiteral("content.xml"),
		writeDocument(document));

	zip.addFile(QStringLiteral("styles.xml"),
		writeStylesDocument(document));

	zip.close();

	return zip.status() == QtZipWriter::NoError;
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeUncompressed(QIODevice* device, const QTextDocument* document)
{
	m_xml.setDevice(device);
	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:office:1.0"), QStringLiteral("office"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:style:1.0"), QStringLiteral("style"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:text:1.0"), QStringLiteral("text"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"), QStringLiteral("fo"));

	m_xml.writeStartDocument();
	m_xml.writeStartElement(QStringLiteral("office:document"));
	m_xml.writeAttribute(QStringLiteral("office:mimetype"), QStringLiteral("application/vnd.oasis.opendocument.text"));
	m_xml.writeAttribute(QStringLiteral("office:version"), QStringLiteral("1.2"));

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
	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:office:1.0"), QStringLiteral("office"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:style:1.0"), QStringLiteral("style"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:text:1.0"), QStringLiteral("text"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"), QStringLiteral("fo"));

	m_xml.writeStartDocument();
	m_xml.writeStartElement(QStringLiteral("office:document-content"));
	m_xml.writeAttribute(QStringLiteral("office:version"), QStringLiteral("1.2"));

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
	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:office:1.0"), QStringLiteral("office"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:style:1.0"), QStringLiteral("style"));
	m_xml.writeNamespace(QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0"), QStringLiteral("fo"));

	m_xml.writeStartDocument();
	m_xml.writeStartElement(QStringLiteral("office:document-styles"));
	m_xml.writeAttribute(QStringLiteral("office:version"), QStringLiteral("1.2"));

	writeStyles(document);

	m_xml.writeEndElement();
	m_xml.writeEndDocument();

	buffer.close();
	return data;
}

//-----------------------------------------------------------------------------

void OdtWriter::writeStyles(const QTextDocument*)
{
	static const QList<QStringList> styles{
		{"Normal", "Normal", "0", "12pt", "normal"},
		{"Heading-1", "Heading 1", "1", "18pt", "bold"},
		{"Heading-2", "Heading 2", "2", "16pt", "bold"},
		{"Heading-3", "Heading 3", "3", "14pt", "bold"},
		{"Heading-4", "Heading 4", "4", "12pt", "bold"},
		{"Heading-5", "Heading 5", "5", "10pt", "bold"},
		{"Heading-6", "Heading 6", "6", "8pt", "bold"}
	};

	m_xml.writeStartElement(QStringLiteral("office:styles"));
	for (const QStringList& style : styles) {
		m_xml.writeStartElement(QStringLiteral("style:style"));
		m_xml.writeAttribute(QStringLiteral("style:name"), style[0]);
		m_xml.writeAttribute(QStringLiteral("style:display-name"), style[1]);
		m_xml.writeAttribute(QStringLiteral("style:family"), QStringLiteral("paragraph"));
		if (style[0] != QStringLiteral("Normal")) {
			m_xml.writeAttribute(QStringLiteral("style:parent-style-name"), QStringLiteral("Normal"));
			m_xml.writeAttribute(QStringLiteral("style:next-style-name"), QStringLiteral("Normal"));
			m_xml.writeAttribute(QStringLiteral("style:default-outline-level"), style[2]);
		}

		m_xml.writeStartElement(QStringLiteral("style:text-properties"));
		m_xml.writeAttribute(QStringLiteral("fo:font-size"), style[3]);
		m_xml.writeAttribute(QStringLiteral("fo:font-weight"), style[4]);
		m_xml.writeEndElement();

		m_xml.writeEndElement();
	}
	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void OdtWriter::writeAutomaticStyles(const QTextDocument* document)
{
	m_xml.writeStartElement(QStringLiteral("office:automatic-styles"));

	const QList<QTextFormat> formats = document->allFormats();

	// Find all used styles
	QList<int> text_styles;
	QList<int> paragraph_styles;
	int index = 0;
	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		index = block.blockFormatIndex();
		if (!paragraph_styles.contains(index)) {
			const int heading = block.blockFormat().property(QTextFormat::UserProperty).toInt();
			if (!heading) {
				paragraph_styles.append(index);
			} else {
				m_styles.insert(index, QString("Heading-%1").arg(heading));
			}
		}
		for (QTextBlock::iterator iter = block.begin(); !iter.atEnd(); ++iter) {
			index = iter.fragment().charFormatIndex();
			if (!text_styles.contains(index) && formats.at(index).propertyCount()) {
				text_styles.append(index);
			}
		}
	}

	// Write text styles
	int text_style = 1;
	for (const int index : std::as_const(text_styles)) {
		const QTextFormat& format = formats.at(index);
		const QString name = QStringLiteral("T") + QString::number(text_style);
		if (writeTextStyle(format.toCharFormat(), name)) {
			m_styles.insert(index, name);
			++text_style;
		}
	}

	// Write paragraph styles
	int paragraph_style = 1;
	for (const int index : std::as_const(paragraph_styles)) {
		const QTextFormat& format = formats.at(index);
		const QString name = QStringLiteral("P") + QString::number(paragraph_style);
		if (writeParagraphStyle(format.toBlockFormat(), name)) {
			m_styles.insert(index, name);
			++paragraph_style;
		} else {
			m_styles.insert(index, QStringLiteral("Normal"));
		}
	}

	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeParagraphStyle(const QTextBlockFormat& format, const QString& name)
{
	QXmlStreamAttributes attributes;
	const bool rtl = format.layoutDirection() == Qt::RightToLeft;
	if (rtl) {
		attributes.append(QStringLiteral("style:writing-mode"), QStringLiteral("rl"));
	}

	const Qt::Alignment align = format.alignment();
	if (rtl && (align & Qt::AlignLeft)) {
		attributes.append(QStringLiteral("fo:text-align"), QStringLiteral("left"));
	} else if (align & Qt::AlignRight) {
		attributes.append(QStringLiteral("fo:text-align"), QStringLiteral("right"));
	} else if (align & Qt::AlignCenter) {
		attributes.append(QStringLiteral("fo:text-align"), QStringLiteral("center"));
	} else if (align & Qt::AlignJustify) {
		attributes.append(QStringLiteral("fo:text-align"), QStringLiteral("justify"));
	}

	if (format.indent() > 0) {
		attributes.append(QStringLiteral("fo:margin-left"), QString::number(format.indent() * 0.5) + QStringLiteral("in"));
	}

	if (attributes.isEmpty()) {
		return false;
	}

	m_xml.writeStartElement(QStringLiteral("style:style"));
	m_xml.writeAttribute(QStringLiteral("style:name"), name);
	m_xml.writeAttribute(QStringLiteral("style:family"), QStringLiteral("paragraph"));
	m_xml.writeAttribute(QStringLiteral("style:parent-style-name"), QStringLiteral("Normal"));

	m_xml.writeEmptyElement(QStringLiteral("style:paragraph-properties"));
	m_xml.writeAttributes(attributes);
	m_xml.writeEndElement();

	return true;
}

//-----------------------------------------------------------------------------

bool OdtWriter::writeTextStyle(const QTextCharFormat& format, const QString& name)
{
	QXmlStreamAttributes attributes;
	if (format.fontWeight() == QFont::Bold) {
		attributes.append(QStringLiteral("fo:font-weight"), QStringLiteral("bold"));
	}
	if (format.fontItalic()) {
		attributes.append(QStringLiteral("fo:font-style"), QStringLiteral("italic"));
	}
	if (format.fontUnderline()) {
		attributes.append(QStringLiteral("style:text-underline-type"), QStringLiteral("single"));
		attributes.append(QStringLiteral("style:text-underline-style"), QStringLiteral("solid"));
	}
	if (format.fontStrikeOut()) {
		attributes.append(QStringLiteral("style:text-line-through-type"), QStringLiteral("single"));
	}
	if (format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
		attributes.append(QStringLiteral("style:text-position"), QStringLiteral("super"));
	} else if (format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
		attributes.append(QStringLiteral("style:text-position"), QStringLiteral("sub"));
	}

	if (attributes.isEmpty()) {
		return false;
	}

	m_xml.writeStartElement(QStringLiteral("style:style"));
	m_xml.writeAttribute(QStringLiteral("style:name"), name);
	m_xml.writeAttribute(QStringLiteral("style:family"), QStringLiteral("text"));

	m_xml.writeEmptyElement(QStringLiteral("style:text-properties"));
	m_xml.writeAttributes(attributes);
	m_xml.writeEndElement();

	return true;
}

//-----------------------------------------------------------------------------

void OdtWriter::writeBody(const QTextDocument* document)
{
	m_xml.writeStartElement(QStringLiteral("office:body"));
	m_xml.writeStartElement(QStringLiteral("office:text"));

	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		const int heading = block.blockFormat().property(QTextFormat::UserProperty).toInt();
		if (!heading) {
			m_xml.writeStartElement(QStringLiteral("text:p"));
		} else {
			m_xml.writeStartElement(QStringLiteral("text:h"));
			m_xml.writeAttribute(QStringLiteral("text:outline-level"), QString::number(heading));
		}
		m_xml.writeAttribute(QStringLiteral("text:style-name"), m_styles.value(block.blockFormatIndex()));
		m_xml.setAutoFormatting(false);

		for (QTextBlock::iterator iter = block.begin(); !iter.atEnd(); ++iter) {
			const QTextFragment fragment = iter.fragment();
			const QString style = m_styles.value(fragment.charFormatIndex());
			if (!style.isEmpty()) {
				m_xml.writeStartElement(QStringLiteral("text:span"));
				m_xml.writeAttribute(QStringLiteral("text:style-name"), style);
			}

			const QString text = fragment.text();
			int start = 0;
			int spaces = -1;
			for (int i = 0, count = text.length(); i < count; ++i) {
				const QChar c = text.at(i);
				if (c.unicode() == 0x0) {
					m_xml.writeCharacters(text.mid(start, i - start));
					spaces = -1;
					start = i + 1;
				} else if (c.unicode() == 0x0009) {
					m_xml.writeCharacters(text.mid(start, i - start));
					m_xml.writeEmptyElement(QStringLiteral("text:tab"));
					spaces = -1;
					start = i + 1;
				} else if (c.unicode() == 0x2028) {
					m_xml.writeCharacters(text.mid(start, i - start));
					m_xml.writeEmptyElement(QStringLiteral("text:line-break"));
					spaces = -1;
					start = i + 1;
				} else if (c.unicode() == 0x0020) {
					++spaces;
				} else if (spaces > 0) {
					m_xml.writeCharacters(text.mid(start, i - spaces - start));
					m_xml.writeEmptyElement(QStringLiteral("text:s"));
					m_xml.writeAttribute(QStringLiteral("text:c"), QString::number(spaces));
					spaces = -1;
					start = i;
				} else {
					spaces = -1;
				}
			}
			if (spaces > 0) {
				m_xml.writeCharacters(text.mid(start, text.length() - spaces - start));
				m_xml.writeEmptyElement(QStringLiteral("text:s"));
				m_xml.writeAttribute(QStringLiteral("text:c"), QString::number(spaces));
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
