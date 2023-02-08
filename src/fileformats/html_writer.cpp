/*
	SPDX-FileCopyrightText: 2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "html_writer.h"

#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>

//-----------------------------------------------------------------------------

bool HtmlWriter::write(QIODevice* device, const QTextDocument* document)
{
	m_xml.setDevice(device);

	m_xml.writeDTD(QStringLiteral("<!DOCTYPE html>\n"));

	m_xml.setAutoFormatting(true);
	m_xml.setAutoFormattingIndent(1);

	m_xml.writeStartElement(QStringLiteral("html"));

	m_xml.writeStartElement(QStringLiteral("head"));
	m_xml.writeEmptyElement(QStringLiteral("meta"));
	m_xml.writeAttribute(QStringLiteral("charset"), QStringLiteral("utf-8"));
	m_xml.writeEndElement();

	m_xml.writeStartElement(QStringLiteral("body"));

	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		const QTextBlockFormat block_format = block.blockFormat();
		int block_format_elements = 0;
		for (int i = 0, count = block_format.indent(); i < count; ++i) {
			m_xml.writeStartElement(QStringLiteral("blockquote"));
			++block_format_elements;
		}

		const int heading = block.blockFormat().property(QTextFormat::UserProperty).toInt();
		m_xml.writeStartElement(!heading ? QStringLiteral("p") : QString("h%1").arg(heading));
		{
			const bool rtl = block_format.layoutDirection() == Qt::RightToLeft;
			if (rtl) {
				m_xml.writeAttribute(QStringLiteral("dir"), QStringLiteral("rtl"));
			}

			const Qt::Alignment align = block_format.alignment();
			if (rtl && (align & Qt::AlignLeft)) {
				m_xml.writeAttribute(QStringLiteral("style"), QStringLiteral("text-align:left"));
			} else if (align & Qt::AlignRight) {
				m_xml.writeAttribute(QStringLiteral("style"), QStringLiteral("text-align:right"));
			} else if (align & Qt::AlignCenter) {
				m_xml.writeAttribute(QStringLiteral("style"), QStringLiteral("text-align:center"));
			} else if (align & Qt::AlignJustify) {
				m_xml.writeAttribute(QStringLiteral("style"), QStringLiteral("text-align:justify"));
			}
		}
		m_xml.setAutoFormatting(false);

		int length = 0;

		for (QTextBlock::iterator iter = block.begin(); !iter.atEnd(); ++iter) {
			const QTextFragment fragment = iter.fragment();

			const int end = fragment.length();
			if (!end) {
				continue;
			}
			length += end;

			const QTextCharFormat format = fragment.charFormat();
			int char_format_elements = 0;
			if (format.fontWeight() == QFont::Bold) {
				m_xml.writeStartElement(QStringLiteral("b"));
				++char_format_elements;
			}
			if (format.fontItalic()) {
				m_xml.writeStartElement(QStringLiteral("i"));
				++char_format_elements;
			}
			if (format.fontUnderline()) {
				m_xml.writeStartElement(QStringLiteral("u"));
				++char_format_elements;
			}
			if (format.fontStrikeOut()) {
				m_xml.writeStartElement(QStringLiteral("s"));
				++char_format_elements;
			}
			if (format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
				m_xml.writeStartElement(QStringLiteral("sup"));
				++char_format_elements;
			} else if (format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
				m_xml.writeStartElement(QStringLiteral("sub"));
				++char_format_elements;
			}

			const QString text = fragment.text();
			int start = 0;
			for (int i = 0; i < end; ++i) {
				if (text.at(i).unicode() == 0x2028) {
					m_xml.writeCharacters(text.mid(start, i - start));
					m_xml.writeEmptyElement(QStringLiteral("br"));
					start = i + 1;
				}
			}
			m_xml.writeCharacters(text.mid(start));

			for (int i = 0; i < char_format_elements; ++i) {
				m_xml.writeEndElement();
			}
		}

		if (!length) {
			m_xml.writeEmptyElement(QStringLiteral("br"));
		}

		m_xml.writeEndElement();
		m_xml.setAutoFormatting(true);

		for (int i = 0; i < block_format_elements; ++i) {
			m_xml.writeEndElement();
		}
	}
	m_xml.writeEndElement();

	m_xml.writeEndElement();
	m_xml.writeEndDocument();

	return true;
}

//-----------------------------------------------------------------------------
