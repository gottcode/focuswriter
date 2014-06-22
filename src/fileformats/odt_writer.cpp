/***********************************************************************
 *
 * Copyright (C) 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

OdtWriter::OdtWriter()
{
}

//-----------------------------------------------------------------------------

bool OdtWriter::write(QIODevice* device, const QTextDocument* document)
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
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<office:document-styles "
		"xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
		"xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
		"xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" "
		"office:version=\"1.2\">\n"
		" <office:styles>\n"
		"  <style:style style:name=\"Normal\" style:display-name=\"Normal\" style:family=\"paragraph\">\n"
		"   <style:text-properties fo:font-size=\"12pt\" fo:font-weight=\"normal\"/>\n"
		"  </style:style>\n"
		"  <style:style style:name=\"Heading-1\" style:display-name=\"Heading 1\" style:family=\"paragraph\" style:parent-style-name=\"Normal\" style:next-style-name=\"Normal\" style:default-outline-level=\"1\">\n"
		"   <style:text-properties fo:font-size=\"18pt\" fo:font-weight=\"bold\"/>\n"
		"  </style:style>\n"
		"  <style:style style:name=\"Heading-2\" style:display-name=\"Heading 2\" style:family=\"paragraph\" style:parent-style-name=\"Normal\" style:next-style-name=\"Normal\" style:default-outline-level=\"2\">\n"
		"   <style:text-properties fo:font-size=\"16pt\" fo:font-weight=\"bold\"/>\n"
		"  </style:style>\n"
		"  <style:style style:name=\"Heading-3\" style:display-name=\"Heading 3\" style:family=\"paragraph\" style:parent-style-name=\"Normal\" style:next-style-name=\"Normal\" style:default-outline-level=\"3\">\n"
		"   <style:text-properties fo:font-size=\"14pt\" fo:font-weight=\"bold\"/>\n"
		"  </style:style>\n"
		"  <style:style style:name=\"Heading-4\" style:display-name=\"Heading 4\" style:family=\"paragraph\" style:parent-style-name=\"Normal\" style:next-style-name=\"Normal\" style:default-outline-level=\"4\">\n"
		"   <style:text-properties fo:font-size=\"12pt\" fo:font-weight=\"bold\"/>\n"
		"  </style:style>\n"
		"  <style:style style:name=\"Heading-5\" style:display-name=\"Heading 5\" style:family=\"paragraph\" style:parent-style-name=\"Normal\" style:next-style-name=\"Normal\" style:default-outline-level=\"5\">\n"
		"   <style:text-properties fo:font-size=\"10pt\" fo:font-weight=\"bold\"/>\n"
		"  </style:style>\n"
		"  <style:style style:name=\"Heading-6\" style:display-name=\"Heading 6\" style:family=\"paragraph\" style:parent-style-name=\"Normal\" style:next-style-name=\"Normal\" style:default-outline-level=\"6\">\n"
		"   <style:text-properties fo:font-size=\"8pt\" fo:font-weight=\"bold\"/>\n"
		"  </style:style>\n"
		" </office:styles>\n"
		"</office:document-styles>\n");

	zip.close();

	return zip.status() == QtZipWriter::NoError;
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
	int text_style = 0;
	for (int i = 0; i < text_styles.size(); ++i) {
		int index = text_styles.at(i);
		const QTextFormat& format = formats.at(index);
		QString name = QString::fromLatin1("T") + QString::number(++text_style);
		m_styles.insert(index, name);
		writeTextStyle(format.toCharFormat(), name);
	}

	// Write paragraph styles
	int paragraph_style = 0;
	for (int i = 0; i < paragraph_styles.size(); ++i) {
		int index = paragraph_styles.at(i);
		const QTextFormat& format = formats.at(index);
		QString name = QString::fromLatin1("P") + QString::number(++paragraph_style);
		m_styles.insert(index, name);
		writeParagraphStyle(format.toBlockFormat(), name);
	}

	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void OdtWriter::writeParagraphStyle(const QTextBlockFormat& format, const QString& name)
{
	m_xml.writeStartElement(QString::fromLatin1("style:style"));
	m_xml.writeAttribute(QString::fromLatin1("style:name"), name);
	m_xml.writeAttribute(QString::fromLatin1("style:family"), QString::fromLatin1("paragraph"));
	m_xml.writeAttribute(QString::fromLatin1("style:parent-style-name"), QString::fromLatin1("Normal"));

	m_xml.writeEmptyElement(QString::fromLatin1("style:paragraph-properties"));

	bool rtl = format.layoutDirection() == Qt::RightToLeft;
	if (rtl) {
		m_xml.writeAttribute(QString::fromLatin1("style:writing-mode"), QString::fromLatin1("rl"));
	}

	Qt::Alignment align = format.alignment();
	if (rtl && (align & Qt::AlignLeft)) {
		m_xml.writeAttribute(QString::fromLatin1("fo:text-align"), QString::fromLatin1("left"));
	} else if (align & Qt::AlignRight) {
		m_xml.writeAttribute(QString::fromLatin1("fo:text-align"), QString::fromLatin1("right"));
	} else if (align & Qt::AlignCenter) {
		m_xml.writeAttribute(QString::fromLatin1("fo:text-align"), QString::fromLatin1("center"));
	} else if (align & Qt::AlignJustify) {
		m_xml.writeAttribute(QString::fromLatin1("fo:text-align"), QString::fromLatin1("justify"));
	}

	if (format.indent() > 0) {
		m_xml.writeAttribute(QString::fromLatin1("fo:margin-left"), QString::number(format.indent() * 0.5) + QString::fromLatin1("in"));
	}

	m_xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void OdtWriter::writeTextStyle(const QTextCharFormat& format, const QString& name)
{
	m_xml.writeStartElement(QString::fromLatin1("style:style"));
	m_xml.writeAttribute(QString::fromLatin1("style:name"), name);
	m_xml.writeAttribute(QString::fromLatin1("style:family"), QString::fromLatin1("text"));

	m_xml.writeEmptyElement(QString::fromLatin1("style:text-properties"));

	if (format.fontWeight() == QFont::Bold) {
		m_xml.writeAttribute(QString::fromLatin1("fo:font-weight"), QString::fromLatin1("bold"));
	}
	if (format.fontItalic()) {
		m_xml.writeAttribute(QString::fromLatin1("fo:font-style"), QString::fromLatin1("italic"));
	}
	if (format.fontUnderline()) {
		m_xml.writeAttribute(QString::fromLatin1("style:text-underline-type"), QString::fromLatin1("single"));
		m_xml.writeAttribute(QString::fromLatin1("style:text-underline-style"), QString::fromLatin1("solid"));
	}
	if (format.fontStrikeOut()) {
		m_xml.writeAttribute(QString::fromLatin1("style:text-line-through-type"), QString::fromLatin1("single"));
	}
	if (format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
		m_xml.writeAttribute(QString::fromLatin1("style:text-position"), QString::fromLatin1("super"));
	} else if (format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
		m_xml.writeAttribute(QString::fromLatin1("style:text-position"), QString::fromLatin1("sub"));
	}

	m_xml.writeEndElement();
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
				if (c.unicode() == 0x0009) {
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
