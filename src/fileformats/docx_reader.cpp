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

#include "docx_reader.h"

#include <QTextDocument>
#include <QXmlStreamAttributes>
#include <QtZipReader>

#include <cmath>

//-----------------------------------------------------------------------------

static bool readBool(const QStringRef& value)
{
	// ECMA-376, ISO/IEC 29500 strict
	if (value.isEmpty()) {
		return true;
	} else if (value == "false") {
		return false;
	} else if (value == "true") {
		return true;
	} else if (value == "0") {
		return false;
	} else if (value == "1") {
		return true;
	// ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
	} else if (value == "off") {
		return false;
	} else if (value == "on") {
		return true;
	// Invalid, just guess
	} else {
		return true;
	}
}

//-----------------------------------------------------------------------------

DocxReader::DocxReader() :
	m_in_block(false)
{
	m_xml.setNamespaceProcessing(false);
}

//-----------------------------------------------------------------------------

bool DocxReader::canRead(QIODevice* device)
{
	return QtZipReader::canRead(device);
}

//-----------------------------------------------------------------------------

void DocxReader::readData(QIODevice* device)
{
	m_in_block = m_cursor.document()->blockCount();
	m_current_style.block_format = m_cursor.blockFormat();

	// Open archive
	QtZipReader zip(device);

	// Read archive
	if (zip.isReadable()) {
		const QString files[] = { QString::fromLatin1("word/styles.xml"), QString::fromLatin1("word/document.xml") };
		for (int i = 0; i < 2; ++i) {
			QByteArray data = zip.fileData(files[i]);
			if (data.isEmpty()) {
				continue;
			}
			m_xml.addData(data);
			readContent();
			if (m_xml.hasError()) {
				m_error = m_xml.errorString();
				break;
			}
			m_xml.clear();
		}
	} else {
		m_error = tr("Unable to open archive.");
	}

	// Close archive
	zip.close();

	QCoreApplication::processEvents();
}

//-----------------------------------------------------------------------------

void DocxReader::readContent()
{
	m_xml.readNextStartElement();
	if (m_xml.qualifiedName() == "w:styles") {
		readStyles();
	} else if (m_xml.qualifiedName() == "w:document") {
		readDocument();
	}
}

//-----------------------------------------------------------------------------

void DocxReader::readStyles()
{
	if (!m_xml.readNextStartElement()) {
		return;
	}

	// Read document defaults
	if (m_xml.qualifiedName() == "w:docDefaults") {
		while (m_xml.readNextStartElement()) {
			if (m_xml.qualifiedName() == "w:rPrDefault") {
				if (m_xml.readNextStartElement()) {
					if (m_xml.qualifiedName() == "w:rPr") {
						readRunProperties(m_current_style);
					} else {
						m_xml.skipCurrentElement();
					}
				}
			} else if (m_xml.qualifiedName() == "w:pPrDefault") {
				if (m_xml.readNextStartElement()) {
					if (m_xml.qualifiedName() == "w:pPr") {
						readParagraphProperties(m_current_style);
					} else {
						m_xml.skipCurrentElement();
					}
				}
			} else {
				m_xml.skipCurrentElement();
			}
		}
		m_xml.skipCurrentElement();
	}

	// Read styles
	QHash<Style::Type, QString> default_style;
	QHash<QString, QStringList> style_tree;

	do {
		if (m_xml.qualifiedName() == "w:style") {
			Style style;

			// Find style type
			QStringRef type = m_xml.attributes().value("w:type");
			if (type == "paragraph") {
				style.type = Style::Paragraph;
			} else if (type == "character") {
				style.type = Style::Character;
			} else {
				m_xml.skipCurrentElement();
				continue;
			}

			// Find style ID
			QString style_id = m_xml.attributes().value(QLatin1String("w:styleId")).toString();
			if (m_styles.contains(style_id)) {
				m_xml.skipCurrentElement();
				continue;
			}

			// Add style ID to tree
			if (!style_tree.contains(style_id)) {
				style_tree.insert(style_id, QStringList());
			}

			// Determine if this is the default style
			if (m_xml.attributes().hasAttribute("w:default") && readBool(m_xml.attributes().value("w:default"))) {
				default_style[style.type] = style_id;
			}

			// Read style contents
			while (m_xml.readNextStartElement()) {
				if (m_xml.qualifiedName() == "w:name") {
					QString name = m_xml.attributes().value("w:val").toString();
					if (name.startsWith("Head")) {
						int heading = qBound(1, name.at(name.length() - 1).digitValue(), 6);
						style.block_format.setProperty(QTextFormat::UserProperty, heading);
					}
					m_xml.skipCurrentElement();
				} else if (m_xml.qualifiedName() == "w:basedOn") {
					QString parent_style_id = m_xml.attributes().value("w:val").toString();
					if (m_styles.contains(parent_style_id) && (style.type == m_styles[parent_style_id].type)) {
						Style newstyle = m_styles[parent_style_id];
						newstyle.block_format.merge(style.block_format);
						newstyle.char_format.merge(style.char_format);
						style = newstyle;
					}
					style_tree[parent_style_id] += style_id;
					m_xml.skipCurrentElement();
				} else if ((style.type == Style::Paragraph) && (m_xml.qualifiedName() == "w:pPr")) {
					readParagraphProperties(style, false);
				} else if (m_xml.qualifiedName() == "w:rPr") {
					readRunProperties(style, false);
				} else {
					m_xml.skipCurrentElement();
				}
			}

			// Add to style list
			m_styles.insert(style_id, style);

			// Recursively apply style to children
			QStringList children = style_tree.value(style_id);
			while (!children.isEmpty()) {
				QString child_id = children.takeFirst();

				Style newstyle = style;
				Style& childstyle = m_styles[child_id];
				newstyle.merge(childstyle);
				childstyle = newstyle;

				children.append(style_tree.value(child_id));
			}
		} else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
			m_xml.skipCurrentElement();
		}
	} while (m_xml.readNextStartElement());

	// Apply default style
	m_current_style.block_format.merge(m_styles.value(default_style.value(Style::Paragraph)).block_format);
	m_current_style.char_format.merge(m_styles.value(default_style.value(Style::Character)).char_format);
}

//-----------------------------------------------------------------------------

void DocxReader::readDocument()
{
	m_cursor.beginEditBlock();
	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "w:body") {
			readBody();
		} else {
			m_xml.skipCurrentElement();
		}
	}
	m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------

void DocxReader::readBody()
{
	while (m_xml.readNextStartElement()) {
		if (m_xml.qualifiedName() == "w:p") {
			readParagraph();
		} else {
			m_xml.skipCurrentElement();
		}
	}
}

//-----------------------------------------------------------------------------

void DocxReader::readParagraph()
{
	bool has_children = m_xml.readNextStartElement();

	// Style paragraph
	bool changedstate = false;
	if (has_children && (m_xml.qualifiedName() == "w:pPr")) {
		changedstate = true;
		m_previous_styles.push(m_current_style);
		readParagraphProperties(m_current_style);
	}

	// Create paragraph
	if (!m_in_block) {
		m_cursor.insertBlock(m_current_style.block_format, m_current_style.char_format);
		m_in_block = true;
	} else {
		m_cursor.mergeBlockFormat(m_current_style.block_format);
		m_cursor.mergeBlockCharFormat(m_current_style.char_format);
	}

	// Read paragraph text
	if (has_children) {
		do {
			if (m_xml.qualifiedName() == "w:r") {
				readRun();
			} else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
				m_xml.skipCurrentElement();
			}
		} while (m_xml.readNextStartElement());
	}
	m_in_block = false;

	// Reset paragraph styling
	if (changedstate) {
		m_current_style = m_previous_styles.pop();
	}

	QCoreApplication::processEvents();
}

//-----------------------------------------------------------------------------

void DocxReader::readParagraphProperties(Style& style, bool allowstyles)
{
	int left_indent = 0, right_indent = 0, indent = 0;
	bool textdir = false;
	while (m_xml.readNextStartElement()) {
		QStringRef value = m_xml.attributes().value("w:val");
		if (m_xml.qualifiedName() == "w:jc") {
			// ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
			if (value == "left") {
				style.block_format.setAlignment(!textdir ? Qt::AlignLeading : (Qt::AlignLeft | Qt::AlignAbsolute));
			} else if (value == "right") {
				style.block_format.setAlignment(!textdir ? Qt::AlignTrailing : (Qt::AlignRight | Qt::AlignAbsolute));
			// ECMA-376, ISO/IEC 29500 strict
			} else if (value == "center") {
				style.block_format.setAlignment(Qt::AlignCenter);
			} else if (value == "both") {
				style.block_format.setAlignment(Qt::AlignJustify);
			// ECMA-376 2nd edition, ISO/IEC 29500 strict
			} else if (value == "start") {
				style.block_format.setAlignment(Qt::AlignLeading);
			} else if (value == "end") {
				style.block_format.setAlignment(Qt::AlignTrailing);
			}
		} else if (m_xml.qualifiedName() == "w:ind") {
			// ECMA-376 1st edition, ECMA-376 2nd edition transitional, ISO/IEC 29500 transitional
			left_indent = std::lround(m_xml.attributes().value("w:left").toString().toDouble() / 720.0);
			right_indent = std::lround(m_xml.attributes().value("w:right").toString().toDouble() / 720.0);
			// ECMA-376 2nd edition, ISO/IEC 29500 strict
			indent = std::lround(m_xml.attributes().value("w:start").toString().toDouble() / 720.0);
			if (indent) {
				style.block_format.setIndent(indent);
				left_indent = right_indent = 0;
			}
		} else if (m_xml.qualifiedName() == "w:bidi") {
			if (readBool(m_xml.attributes().value("w:val"))) {
				style.block_format.setLayoutDirection(Qt::RightToLeft);
			}
		} else if (m_xml.qualifiedName() == "w:textDirection") {
			if (value == "rl") {
				textdir = true;
				style.block_format.setLayoutDirection(Qt::RightToLeft);
			} else if (value == "lr") {
				style.block_format.setLayoutDirection(Qt::LeftToRight);
			}
		} else if (m_xml.qualifiedName() == "w:outlineLvl") {
			int heading = m_xml.attributes().value("w:val").toString().toInt();
			if (heading != 9) {
				style.block_format.setProperty(QTextFormat::UserProperty, qBound(1, heading + 1, 6));
			}
		} else if ((m_xml.qualifiedName() == "w:pStyle") && allowstyles) {
			Style pstyle = m_styles.value(value.toString());
			pstyle.merge(style);
			style = pstyle;
		} else if (m_xml.qualifiedName() == "w:rPr") {
			readRunProperties(style);
			continue;
		}

		m_xml.skipCurrentElement();
	}

	if (!indent) {
		if (style.block_format.layoutDirection() != Qt::RightToLeft) {
			if (left_indent) {
				style.block_format.setIndent(left_indent);
			}
		} else {
			if (right_indent) {
				style.block_format.setIndent(right_indent);
			}
		}
	}

	if (style.block_format.property(QTextFormat::UserProperty).toInt()) {
		style.char_format.setFontWeight(QFont::Normal);
	}
}

//-----------------------------------------------------------------------------

void DocxReader::readRun()
{
	if (m_xml.readNextStartElement()) {
		// Style text run
		bool changedstate = false;
		if (m_xml.qualifiedName() == "w:rPr") {
			changedstate = true;
			m_previous_styles.push(m_current_style);
			readRunProperties(m_current_style);
		}

		// Read text run
		do {
			if (m_xml.qualifiedName() == "w:t") {
				readText();
			} else if (m_xml.qualifiedName() == "w:tab") {
				m_cursor.insertText(QChar(0x0009), m_current_style.char_format);
				m_xml.skipCurrentElement();
			} else if (m_xml.qualifiedName() == "w:br") {
				m_cursor.insertText(QChar(0x2028), m_current_style.char_format);
				m_xml.skipCurrentElement();
			} else if (m_xml.qualifiedName() == "w:cr") {
				m_cursor.insertText(QChar(0x2028), m_current_style.char_format);
				m_xml.skipCurrentElement();
			} else if (m_xml.tokenType() != QXmlStreamReader::EndElement) {
				m_xml.skipCurrentElement();
			}
		} while (m_xml.readNextStartElement());

		// Reset text run styling
		if (changedstate) {
			m_current_style = m_previous_styles.pop();
		}
	}
}

//-----------------------------------------------------------------------------

void DocxReader::readRunProperties(Style& style, bool allowstyles)
{
	while (m_xml.readNextStartElement()) {
		QStringRef value = m_xml.attributes().value("w:val");
		if ((m_xml.qualifiedName() == "w:b") || (m_xml.qualifiedName() == "w:bCs")) {
			style.char_format.setFontWeight(readBool(value) ? QFont::Bold : QFont::Normal);
		} else if ((m_xml.qualifiedName() == "w:i") || (m_xml.qualifiedName() == "w:iCs")) {
			style.char_format.setFontItalic(readBool(value));
		} else if (m_xml.qualifiedName() == "w:u") {
			if (value == "single") {
				style.char_format.setFontUnderline(true);
			} else if (value == "none") {
				style.char_format.setFontUnderline(false);
			} else if ((value == "dash")
					|| (value == "dashDotDotHeavy")
					|| (value == "dashDotHeavy")
					|| (value == "dashedHeavy")
					|| (value == "dashLong")
					|| (value == "dashLongHeavy")
					|| (value == "dotDash")
					|| (value == "dotDotDash")
					|| (value == "dotted")
					|| (value == "dottedHeavy")
					|| (value == "double")
					|| (value == "thick")
					|| (value == "wave")
					|| (value == "wavyDouble")
					|| (value == "wavyHeavy")
					|| (value == "words")) {
				style.char_format.setFontUnderline(true);
			}
		} else if (m_xml.qualifiedName() == "w:strike") {
			style.char_format.setFontStrikeOut(readBool(value));
		} else if (m_xml.qualifiedName() == "w:vertAlign") {
			if (value == "superscript") {
				style.char_format.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
			} else if (value == "subscript") {
				style.char_format.setVerticalAlignment(QTextCharFormat::AlignSubScript);
			} else if (value == "baseline") {
				style.char_format.setVerticalAlignment(QTextCharFormat::AlignNormal);
			}
		} else if ((m_xml.qualifiedName() == "w:rStyle") && allowstyles) {
			Style rstyle = m_styles.value(value.toString());
			rstyle.merge(style);
			style = rstyle;
		}

		m_xml.skipCurrentElement();
	}
}

//-----------------------------------------------------------------------------

void DocxReader::readText()
{
	bool keepws = (m_xml.attributes().value("xml:space") == "preserve");

	QString text;
	while (m_xml.readNext() == QXmlStreamReader::Characters) {
		if (keepws || !m_xml.isWhitespace()) {
			text += m_xml.text();
		}
	}
	if (!text.isEmpty()) {
		m_cursor.insertText(text, m_current_style.char_format);
	}
}

//-----------------------------------------------------------------------------
