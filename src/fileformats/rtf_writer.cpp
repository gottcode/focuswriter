/*
	SPDX-FileCopyrightText: 2010-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "rtf_writer.h"

#include <QHash>
#include <QIODevice>
#include <QLocale>
#include <QTextBlock>
#include <QTextDocument>

//-----------------------------------------------------------------------------

bool RtfWriter::write(QIODevice* device, const QTextDocument* text)
{
	device->write("{\\rtf1\\ansi\\ansicpg819\n"
		"{\\stylesheet\n"
		"{\\s0 Normal;}\n"
		"{\\s1\\snext0\\outlinelevel0\\fs36\\b Heading 1;}\n"
		"{\\s2\\snext0\\outlinelevel1\\fs32\\b Heading 2;}\n"
		"{\\s3\\snext0\\outlinelevel2\\fs28\\b Heading 3;}\n"
		"{\\s4\\snext0\\outlinelevel3\\fs24\\b Heading 4;}\n"
		"{\\s5\\snext0\\outlinelevel4\\fs20\\b Heading 5;}\n"
		"{\\s6\\snext0\\outlinelevel5\\fs16\\b Heading 6;}\n"
		"}\n");

	for (QTextBlock block = text->begin(); block.isValid(); block = block.next()) {
		QByteArray par("{\\pard\\plain");
		const QTextBlockFormat block_format = block.blockFormat();
		const int heading = block_format.property(QTextFormat::UserProperty).toInt();
		if (heading) {
			par += "\\s" + QByteArray::number(heading)
				+ "\\outlinelevel" + QByteArray::number(heading - 1)
				+ "\\fs" + QByteArray::number((10 - heading) * 4)
				+ "\\b";
		} else {
			par += "\\s0";
		}
		const bool rtl = block_format.layoutDirection() == Qt::RightToLeft;
		if (rtl) {
			par += "\\rtlpar";
		}
		const Qt::Alignment align = block_format.alignment();
		if (rtl && (align & Qt::AlignLeft)) {
			par += "\\ql";
		} else if (align & Qt::AlignRight) {
			par += "\\qr";
		} else if (align & Qt::AlignCenter) {
			par += "\\qc";
		} else if (align & Qt::AlignJustify) {
			par += "\\qj";
		}
		if (block_format.indent() > 0) {
			par += "\\li" + QByteArray::number(block_format.indent() * 720);
		}
		device->write(par);

		if (block.begin() != block.end()) {
			device->write(" ");
			for (QTextBlock::iterator iter = block.begin(); !iter.atEnd(); ++iter) {
				const QTextFragment fragment = iter.fragment();
				const QTextCharFormat char_format = fragment.charFormat();
				QByteArray style;
				if (char_format.fontWeight() == QFont::Bold) {
					style += "\\b";
				}
				if (char_format.fontItalic()) {
					style += "\\i";
				}
				if (char_format.fontUnderline()) {
					style += "\\ul";
				}
				if (char_format.fontStrikeOut()) {
					style += "\\strike";
				}
				if (char_format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
					style += "\\super";
				} else if (char_format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
					style += "\\sub";
				}

				if (!style.isEmpty()) {
					device->write("{" + style + " " + fromUnicode(fragment.text()) + "}");
				} else {
					device->write(fromUnicode(fragment.text()));
				}
			}
		}

		device->write("\\par}\n");
	}

	device->write("}");
	return true;
}

//-----------------------------------------------------------------------------

QByteArray RtfWriter::fromUnicode(const QString& string) const
{
	QByteArray text;
	text.reserve(string.length() * 3);

	for (const QChar& c : string) {
		const int unicode = c.unicode();
		switch (unicode) {
		case '\t': text += "\\tab "; break;
		case '\\': text += "\\'5c"; break;
		case '{': text += "\\'7b"; break;
		case '}': text += "\\'7d"; break;
		case 0x00a0: text += "\\~"; break;
		case 0x00ad: text += "\\-"; break;
		case 0x00b7: text += "\\|"; break;
		case 0x2002: text += "\\enspace "; break;
		case 0x2003: text += "\\emspace "; break;
		case 0x2004: text += "\\qmspace "; break;
		case 0x200c: text += "\\zwnj "; break;
		case 0x200d: text += "\\zwj "; break;
		case 0x200e: text += "\\ltrmark "; break;
		case 0x200f: text += "\\rtlmark "; break;
		case 0x2011: text += "\\_"; break;
		case 0x2013: text += "\\endash "; break;
		case 0x2014: text += "\\emdash "; break;
		case 0x2018: text += "\\lquote "; break;
		case 0x2019: text += "\\rquote "; break;
		case 0x201c: text += "\\ldblquote "; break;
		case 0x201d: text += "\\rdblquote "; break;
		case 0x2022: text += "\\bullet "; break;
		case 0x2028: text += "\\line "; break;
		default:
			if ((unicode >= 0x0020) && (unicode < 0x0080)) {
				text += unicode;
			} else {
				text += "\\u";
				text += QByteArray::number((unicode < 0x8000) ? unicode : (unicode - 0x10000));
				text += "?";
			}
		}
	}

	return text;
}

//-----------------------------------------------------------------------------
