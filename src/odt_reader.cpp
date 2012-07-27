/***********************************************************************
 *
 * Copyright (C) 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "odt_reader.h"

#include <QApplication>
#include <QFile>
#include <QTextDocument>

#include <zip.h>

//-----------------------------------------------------------------------------

ODT::Reader::Reader()
	: m_in_block(true)
{
	m_xml.setNamespaceProcessing(false);
}

//-----------------------------------------------------------------------------

QString ODT::Reader::errorString() const
{
	return m_error;
}

//-----------------------------------------------------------------------------

bool ODT::Reader::hasError() const
{
	return !m_error.isEmpty();
}

//-----------------------------------------------------------------------------

void ODT::Reader::read(const QString& filename, QTextDocument* text)
{
	m_filename = filename;

	m_in_block = text->blockCount();
	m_cursor = QTextCursor(text);
	m_block_format = m_cursor.blockFormat();
	m_cursor.beginEditBlock();

	// Open archive
	zip* archive = zip_open(QFile::encodeName(m_filename).constData(), 0, 0);
	if (!archive) {
		m_error = tr("Unable to open archive.");
	}

	try {
		const size_t buffer_size = 0x4000;
		char buffer[buffer_size + 1];

		const char* files[] = { "styles.xml", "content.xml" };
		for (int i = 0; i < 2; ++i) {
			const char* file = files[i];
			int index = zip_name_locate(archive, file, 0);
			if (index != -1) {
				zip_file* zfile = zip_fopen_index(archive, index, 0);
				if (zfile == 0) {
					throw tr("Unable to open file '%1'.").arg(file);
				}

				m_xml.clear();
				int len = 0;
				while ((len = zip_fread(zfile, &buffer, buffer_size)) > 0) {
					buffer[len] = 0;
					m_xml.addData(buffer);
				}

				if (zip_fclose(zfile) != 0) {
					throw tr("Unable to close file '%1'.").arg(file);
				}

				readDocument();
				if (m_xml.hasError()) {
					throw m_xml.errorString();
				}
			}
		}
	}
	catch (QString error) {
		m_error = error;
	}
	m_cursor.endEditBlock();

	// Close archive
	zip_close(archive);
	m_xml.clear();

	QApplication::processEvents();
}

//-----------------------------------------------------------------------------

void ODT::Reader::readDocument()
{
	m_xml.readNextStartElement();
	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "office:styles") {
			readStylesGroup();
		} else if (m_xml.qualifiedName() == "office:automatic-styles") {
			readStylesGroup();
		} else if (m_xml.qualifiedName() == "office:body") {
			readBody();
		}  else {
			m_xml.skipCurrentElement();
		}
	}
}

//-----------------------------------------------------------------------------

void ODT::Reader::readStylesGroup()
{
	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "style:style") {
			readStyle();
		} else {
			m_xml.skipCurrentElement();
		}
	}
}

//-----------------------------------------------------------------------------

void ODT::Reader::readStyle()
{
	QXmlStreamAttributes attributes = m_xml.attributes();

	QString name = attributes.value(QLatin1String("style:name")).toString();

	int type = -1;
	QStringRef family = attributes.value(QLatin1String("style:family"));
	if (family == "paragraph") {
		type = 0;
	} else if (family == "text") {
		type = 1;
	} else {
		m_xml.skipCurrentElement();
		return;
	}

	if (!m_styles[type].contains(name)) {
		m_styles[type][name] = Style(m_block_format);
	}
	Style& style = m_styles[type][name];

	QString parent_style = attributes.value(QLatin1String("style:parent-style-name")).toString();
	if (!parent_style.isEmpty()) {
		if (m_styles[0].contains(parent_style)) {
			style = m_styles[0][parent_style];
		} else if (m_styles[1].contains(parent_style)) {
			style = m_styles[1][parent_style];
		}
	}

	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "style:paragraph-properties") {
			readStyleParagraphProperties(style.block_format);
		} else if (m_xml.qualifiedName() == "style:text-properties") {
			readStyleTextProperties(style.char_format);
		} else {
			m_xml.skipCurrentElement();
		}
	}
}

//-----------------------------------------------------------------------------

void ODT::Reader::readStyleParagraphProperties(QTextBlockFormat& format)
{
	QXmlStreamAttributes attributes = m_xml.attributes();

	QStringRef align = attributes.value(QLatin1String("fo:text-align"));
	if (align == "start") {
		format.setAlignment(Qt::AlignLeading);
	} else if (align == "end") {
		format.setAlignment(Qt::AlignTrailing);
	} else if (align == "left") {
		format.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
	} else if (align == "right") {
		format.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
	} else if (align == "center") {
		format.setAlignment(Qt::AlignHCenter);
	} else if (align == "justify") {
		format.setAlignment(Qt::AlignJustify);
	}

	QStringRef direction = attributes.value(QLatin1String("style:writing-mode"));
	if (direction == "rl-tb" || direction == "rl") {
		format.setLayoutDirection(Qt::RightToLeft);
	} else if (direction == "lr-tb" || direction == "lr") {
		format.setLayoutDirection(Qt::LeftToRight);
	}

	if (attributes.hasAttribute(QLatin1String("fo:margin-left"))) {
		QString margin = attributes.value(QLatin1String("fo:margin-left")).toString();
		QString type = margin.right(2);
		margin.chop(2);
		int value = 0;

		// Assume 96 DPI for margin
		if (type == ("pt")) {
			value = qRound(margin.toDouble() / 36.0);
		} else if (type == ("in")) {
			value = qRound(margin.toDouble() * 2.0);
		}
		format.setIndent(value * 48);
	}

	m_xml.skipCurrentElement();
}

//-----------------------------------------------------------------------------

void ODT::Reader::readStyleTextProperties(QTextCharFormat& format)
{
	QXmlStreamAttributes attributes = m_xml.attributes();

	if (attributes.value(QLatin1String("fo:font-weight")) == "bold") {
		format.setFontWeight(QFont::Bold);
	}
	if (attributes.value(QLatin1String("fo:font-style")) == "italic") {
		format.setFontItalic(true);
	}
	if (attributes.hasAttribute(QLatin1String("style:text-underline-style")) &&
			attributes.value(QLatin1String("style:text-underline-style")) != "none") {
		format.setFontUnderline(true);
	}
	if (attributes.hasAttribute((QLatin1String("style:text-underline-type"))) &&
			attributes.value(QLatin1String("style:text-underline-type")) != "none") {
		format.setFontUnderline(true);
	}
	if (attributes.hasAttribute((QLatin1String("style:text-line-through-type"))) &&
			attributes.value(QLatin1String("style:text-line-through-type")) != "none") {
		format.setFontStrikeOut(true);
	}

	if (attributes.hasAttribute(QLatin1String("style:text-position"))) {
		QStringRef position = attributes.value((QLatin1String("style:text-position")));
		if (position == "super") {
			format.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
		} else if (position == "sub") {
			format.setVerticalAlignment(QTextCharFormat::AlignSubScript);
		} else {
			QString value = position.toString();
			value.chop(1);
			if (value.toInt() > 0) {
				format.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
			} else {
				format.setVerticalAlignment(QTextCharFormat::AlignSubScript);
			}
		}
	}

	m_xml.skipCurrentElement();
}

//-----------------------------------------------------------------------------

void ODT::Reader::readBody()
{
	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "office:text") {
			readBodyText();
		} else {
			m_xml.skipCurrentElement();
		}
	}
}

//-----------------------------------------------------------------------------

void ODT::Reader::readBodyText()
{
	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "text:p" || m_xml.qualifiedName() == "text:h") {
			readParagraph();
		} else {
			m_xml.skipCurrentElement();
		}
	}
}

//-----------------------------------------------------------------------------

void ODT::Reader::readParagraph()
{
	QTextBlockFormat block_format;
	QTextCharFormat char_format;

	// Style paragraph
	QXmlStreamAttributes attributes = m_xml.attributes();
	if (attributes.hasAttribute(QLatin1String("text:style-name"))) {
		const Style& style = m_styles[0][attributes.value(QLatin1String("text:style-name")).toString()];
		block_format = style.block_format;
		char_format = style.char_format;
	}

	// Create paragraph
	if (!m_in_block) {
		m_cursor.insertBlock(block_format, char_format);
		m_in_block = true;
	} else {
		m_cursor.mergeBlockFormat(block_format);
		m_cursor.mergeBlockCharFormat(char_format);
	}

	// Read paragraph text
	readText();
	m_in_block = false;
}

//-----------------------------------------------------------------------------

void ODT::Reader::readSpan()
{
	QXmlStreamAttributes attributes = m_xml.attributes();

	// Style text
	QTextCharFormat format = m_cursor.charFormat();
	if (attributes.hasAttribute(QLatin1String("text:style-name"))) {
		const Style& style = m_styles[1][attributes.value(QLatin1String("text:style-name")).toString()];
		m_cursor.mergeCharFormat(style.char_format);
	}

	if (attributes.hasAttribute(QLatin1String("text:class-names"))) {
		QStringList styles = attributes.value(QLatin1String("text:class-names")).toString().simplified().split(QLatin1Char(' '), QString::SkipEmptyParts);
		int count = styles.count();
		for (int i = 0; i < count; ++i) {
			const Style& style = m_styles[1][styles.at(i)];
			m_cursor.mergeCharFormat(style.char_format);
		}
	}

	// Read styled text
	readText();

	// Restore previous style
	m_cursor.setCharFormat(format);
}

//-----------------------------------------------------------------------------

void ODT::Reader::readText()
{
	int depth = 1;
	while (depth && (m_xml.readNext() != QXmlStreamReader::Invalid)) {
		if (m_xml.isCharacters()) {
			m_cursor.insertText(m_xml.text().toString());
		} else if (m_xml.isStartElement()) {
			++depth;
			if (m_xml.qualifiedName() == "text:span") {
				readSpan();
				--depth;
			} else if (m_xml.qualifiedName() == "text:s") {
				int spaces = m_xml.attributes().value(QLatin1String("text:c")).toString().toInt();
				m_cursor.insertText(QString(qMax(1, spaces), QLatin1Char(' ')));
			} else if (m_xml.qualifiedName() == "text:tab") {
				m_cursor.insertText(QLatin1String("\t"));
			} else if (m_xml.qualifiedName() == "text:line-break") {
				m_cursor.insertText(QChar(0x2028));
			}
		} else if (m_xml.isEndElement()) {
			--depth;
		}
	}
}

//-----------------------------------------------------------------------------
