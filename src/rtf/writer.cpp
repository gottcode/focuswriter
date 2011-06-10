/***********************************************************************
 *
 * Copyright (C) 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "writer.h"

#include <QIODevice>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextDocument>

//-----------------------------------------------------------------------------

RTF::Writer::Writer()
	: m_codec(0)
{
	setCodec(QTextCodec::codecForLocale());
}

//-----------------------------------------------------------------------------

void RTF::Writer::setCodec(QTextCodec* codec)
{
	if (codec == 0) {
		return;
	}
	m_codec = codec;

	if (codec->mibEnum() == 2009) {
		m_header = "{\\rtf1\\pca\\ansicpg850\n\n";
	} else if (codec->mibEnum() == 2011) {
		m_header = "{\\rtf1\\pc\\ansicpg437\n\n";
	} else if (codec->mibEnum() == 2027) {
		m_header = "{\\rtf1\\mac\\ansicpg10000\n\n";
	} else {
		QByteArray codepage;
		QList<QByteArray> aliases = codec->aliases();
		foreach (const QByteArray& alias, aliases) {
			if (alias.startsWith("CP") || alias.startsWith("cp")) {
				codepage = alias;
				break;
			}
		}
		if (!codepage.isEmpty()) {
			m_header = "{\\rtf1\\ansi\\ansicpg" + codepage.mid(2) + "\n\n";
		} else {
			setCodec(QTextCodec::codecForName("CP1252"));
		}
	}
}

//-----------------------------------------------------------------------------

bool RTF::Writer::write(QIODevice* device, QTextDocument* text)
{
	if (m_codec == 0) {
		return false;
	}

	device->write(m_header);

	for (QTextBlock block = text->begin(); block.isValid(); block = block.next()) {
		QByteArray par("{\\pard\\plain");
		QTextBlockFormat block_format = block.blockFormat();
		bool rtl = block_format.layoutDirection() == Qt::RightToLeft;
		if (rtl) {
			par += "\\rtlpar";
		}
		Qt::Alignment align = block_format.alignment();
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
			for (QTextBlock::iterator iter = block.begin(); iter != block.end(); ++iter) {
				QTextFragment fragment = iter.fragment();
				QTextCharFormat char_format = fragment.charFormat();
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

	device->write("\n}");
	return true;
}

//-----------------------------------------------------------------------------

QByteArray RTF::Writer::fromUnicode(const QString& string) const
{
	QByteArray text;

	QByteArray encoded;
	QTextCodec::ConverterState state;
	state.flags = QTextCodec::ConvertInvalidToNull;

	QString::const_iterator end = string.constEnd();
	for (QString::const_iterator i = string.constBegin(); i != end; ++i) {
		switch (i->unicode()) {
		case '\t': text += "\\tab "; break;
		case '\\': text += "\\\\"; break;
		case '{': text += "\\{"; break;
		case '}': text += "\\}"; break;
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
		default:
			encoded = m_codec->fromUnicode(i, 1, &state);
			if (state.invalidChars == 0) {
				if (encoded.count() == 1 && encoded.at(0) >= 0) {
					text += encoded;
				} else {
					for (int j = 0; j < encoded.count(); ++j) {
						text += "\\'" + QByteArray::number(static_cast<unsigned char>(encoded.at(j)), 16);
					}
				}
			} else {
				text += "\\u" + QByteArray::number(i->unicode()) + "?";
			}
		}
	}

	return text;
}

//-----------------------------------------------------------------------------
