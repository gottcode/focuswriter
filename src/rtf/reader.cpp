/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
 *
 * Derived in part from KWord's rtfimport.cpp
 *  Copyright (C) 2001 Ewald Snel <ewald@rambo.its.tudelft.nl>
 *  Copyright (C) 2001 Tomasz Grobelny <grotk@poczta.onet.pl>
 *  Copyright (C) 2003, 2004 Nicolas GOUTTE <goutte@kde.org>
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

#include "reader.h"

#include <QFile>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextDecoder>
#include <QTextEdit>

//-----------------------------------------------------------------------------

namespace
{
	class Function
	{
	public:
		Function(void (RTF::Reader::*func)(qint32) = 0, qint32 value = 0)
			: m_func(func),
			m_value(value)
		{
		}

		void call(RTF::Reader* reader, const RTF::Tokenizer& token) const
		{
			(reader->*m_func)(token.hasValue() ? token.value() : m_value);
		}

	private:
		void (RTF::Reader::*m_func)(qint32);
		qint32 m_value;
	};
	QHash<QByteArray, Function> functions;

	QTextCodec* codecForCodePage(qint32 value, QByteArray* codepage = 0)
	{
		QByteArray name = "CP" + QByteArray::number(value);
		QByteArray codec;
		if (value == 932) {
			codec = "Shift-JIS";
		} else if (value == 10000) {
			codec = "Apple Roman";
		} else if (value == 65001) {
			codec = "UTF-8";
		} else {
			codec = name;
		}
		if (codepage) {
			*codepage = name;
		}
		return QTextCodec::codecForName(codec);
	}
}

//-----------------------------------------------------------------------------

RTF::Reader::Reader()
	: m_in_block(true),
	m_codec(0),
	m_decoder(0)
{
	if (functions.isEmpty()) {
		functions["\\"] = Function(&Reader::insertSymbol, '\\');
		functions["_"] = Function(&Reader::insertSymbol, 0x2011);
		functions["{"] = Function(&Reader::insertSymbol, '{');
		functions["|"] = Function(&Reader::insertSymbol, 0x00b7);
		functions["}"] = Function(&Reader::insertSymbol, '}');
		functions["~"] = Function(&Reader::insertSymbol, 0x00a0);
		functions["-"] = Function(&Reader::insertSymbol, 0x00ad);

		functions["bullet"] = Function(&Reader::insertSymbol, 0x2022);
		functions["emdash"] = Function(&Reader::insertSymbol, 0x2014);
		functions["emspace"] = Function(&Reader::insertSymbol, 0x2003);
		functions["endash"] = Function(&Reader::insertSymbol, 0x2013);
		functions["enspace"] = Function(&Reader::insertSymbol, 0x2002);
		functions["ldblquote"] = Function(&Reader::insertSymbol, 0x201c);
		functions["lquote"] = Function(&Reader::insertSymbol, 0x2018);
		functions["line"] = Function(&Reader::insertSymbol, 0x2028);
		functions["ltrmark"] = Function(&Reader::insertSymbol, 0x200e);
		functions["qmspace"] = Function(&Reader::insertSymbol, 0x2004);
		functions["rdblquote"] = Function(&Reader::insertSymbol, 0x201d);
		functions["rquote"] = Function(&Reader::insertSymbol, 0x2019);
		functions["rtlmark"] = Function(&Reader::insertSymbol, 0x200f);
		functions["tab"] = Function(&Reader::insertSymbol, 0x0009);
		functions["zwj"] = Function(&Reader::insertSymbol, 0x200d);
		functions["zwnj"] = Function(&Reader::insertSymbol, 0x200c);

		functions["\'"] = Function(&Reader::insertHexSymbol);
		functions["u"] = Function(&Reader::insertUnicodeSymbol);
		functions["uc"] = Function(&Reader::setSkipCharacters);
		functions["par"] = Function(&Reader::endBlock);
		functions["\n"] = Function(&Reader::endBlock);
		functions["\r"] = Function(&Reader::endBlock);

		functions["pard"] = Function(&Reader::resetBlockFormatting);
		functions["plain"] = Function(&Reader::resetTextFormatting);

		functions["qc"] = Function(&Reader::setBlockAlignment, Qt::AlignHCenter);
		functions["qj"] = Function(&Reader::setBlockAlignment, Qt::AlignJustify);
		functions["ql"] = Function(&Reader::setBlockAlignment, Qt::AlignLeft | Qt::AlignAbsolute);
		functions["qr"] = Function(&Reader::setBlockAlignment, Qt::AlignRight | Qt::AlignAbsolute);

		functions["li"] = Function(&Reader::setBlockIndent);

		functions["ltrpar"] = Function(&Reader::setBlockDirection, Qt::LeftToRight);
		functions["rtlpar"] = Function(&Reader::setBlockDirection, Qt::RightToLeft);

		functions["b"] = Function(&Reader::setTextBold, true);
		functions["i"] = Function(&Reader::setTextItalic, true);
		functions["strike"] = Function(&Reader::setTextStrikeOut, true);
		functions["striked"] = Function(&Reader::setTextStrikeOut, true);
		functions["ul"] = Function(&Reader::setTextUnderline, true);
		functions["uld"] = Function(&Reader::setTextUnderline, true);
		functions["uldash"] = Function(&Reader::setTextUnderline, true);
		functions["uldashd"] = Function(&Reader::setTextUnderline, true);
		functions["uldb"] = Function(&Reader::setTextUnderline, true);
		functions["ulnone"] = Function(&Reader::setTextUnderline, false);
		functions["ulth"] = Function(&Reader::setTextUnderline, true);
		functions["ulw"] = Function(&Reader::setTextUnderline, true);
		functions["ulwave"] = Function(&Reader::setTextUnderline, true);
		functions["ulhwave"] = Function(&Reader::setTextUnderline, true);
		functions["ululdbwave"] = Function(&Reader::setTextUnderline, true);

		functions["sub"] = Function(&Reader::setTextVerticalAlignment, QTextCharFormat::AlignSubScript);
		functions["super"] = Function(&Reader::setTextVerticalAlignment, QTextCharFormat::AlignSuperScript);
		functions["nosupersub"] = Function(&Reader::setTextVerticalAlignment, QTextCharFormat::AlignNormal);

		functions["ansicpg"] = Function(&Reader::setCodepage);
		functions["ansi"] = Function(&Reader::setCodepage, 1252);
		functions["mac"] = Function(&Reader::setCodepage, 10000);
		functions["pc"] = Function(&Reader::setCodepage, 850);
		functions["pca"] = Function(&Reader::setCodepage, 850);

		functions["deff"] = Function(&Reader::setFont);
		functions["f"] = Function(&Reader::setFont);
		functions["cpg"] = Function(&Reader::setFontCodepage);
		functions["fcharset"] = Function(&Reader::setFontCharset);

		functions["filetbl"] = Function(&Reader::ignoreGroup);
		functions["colortbl"] = Function(&Reader::ignoreGroup);
		functions["fonttbl"] = Function(&Reader::ignoreText);
		functions["stylesheet"] = Function(&Reader::ignoreGroup);
		functions["info"] = Function(&Reader::ignoreGroup);
		functions["*"] = Function(&Reader::ignoreGroup);
	}

	m_state.ignore_control_word = false;
	m_state.ignore_text = false;
	m_state.skip = 1;
	m_state.active_codepage = 0;

	setCodepage(1252);
}

//-----------------------------------------------------------------------------

RTF::Reader::~Reader()
{
	delete m_decoder;
}

//-----------------------------------------------------------------------------

QByteArray RTF::Reader::codePage() const
{
	return m_codepage_name;
}

//-----------------------------------------------------------------------------

QString RTF::Reader::errorString() const
{
	return m_error;
}

//-----------------------------------------------------------------------------

bool RTF::Reader::hasError() const
{
	return !m_error.isEmpty();
}

//-----------------------------------------------------------------------------

void RTF::Reader::read(QIODevice* device, const QTextCursor& cursor)
{
	try {
		// Use theme spacings
		m_block_format = cursor.blockFormat();
		m_state.block_format = m_block_format;

		// Open file
		m_cursor = cursor;
		m_cursor.beginEditBlock();
		m_token.setDevice(device);
		setBlockDirection(Qt::LeftToRight);

		// Check file type
		m_token.readNext();
		if (m_token.type() == StartGroupToken) {
			pushState();
		} else {
			throw tr("Not a supported RTF file.");
		}
		m_token.readNext();
		if (m_token.type() != ControlWordToken || m_token.text() != "rtf" || m_token.value() != 1) {
			throw tr("Not a supported RTF file.");
		}

		// Parse file contents
		while (!m_states.isEmpty() && m_token.hasNext()) {
			m_token.readNext();

			if ((m_token.type() != EndGroupToken) && !m_in_block) {
				m_cursor.insertBlock(m_state.block_format);
				m_in_block = true;
			}

			if (m_token.type() == StartGroupToken) {
				pushState();
			} else if (m_token.type() == EndGroupToken) {
				popState();
			} else if (m_token.type() == ControlWordToken) {
				if (!m_state.ignore_control_word && functions.contains(m_token.text())) {
					functions[m_token.text()].call(this, m_token);
				}
			} else if (m_token.type() == TextToken) {
				if (!m_state.ignore_text) {
					m_cursor.insertText(m_decoder->toUnicode(m_token.text()));
				}
			}
		}
	} catch (const QString& error) {
		m_error = error;
	}
	m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------

void RTF::Reader::endBlock(qint32)
{
	m_in_block = false;
}

//-----------------------------------------------------------------------------

void RTF::Reader::ignoreGroup(qint32)
{
	m_state.ignore_control_word = true;
	m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::ignoreText(qint32)
{
	m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::insertHexSymbol(qint32)
{
	m_cursor.insertText(m_decoder->toUnicode(m_token.hex()));
}

//-----------------------------------------------------------------------------

void RTF::Reader::insertSymbol(qint32 value)
{
	m_cursor.insertText(QChar(value));
}

//-----------------------------------------------------------------------------

void RTF::Reader::insertUnicodeSymbol(qint32 value)
{
	m_cursor.insertText(QChar(value));

	for (int i = m_state.skip; i > 0;) {
		m_token.readNext();

		if (m_token.type() == TextToken) {
			int len = m_token.text().count();
			if (len > i) {
				m_cursor.insertText(m_decoder->toUnicode(m_token.text().mid(i)));
				break;
			} else {
				i -= len;
			}
		} else if (m_token.type() == ControlWordToken) {
			--i;
		} else if (m_token.type() == StartGroupToken) {
			pushState();
			break;
		} else if (m_token.type() == EndGroupToken) {
			popState();
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void RTF::Reader::pushState()
{
	m_states.push(m_state);
}

//-----------------------------------------------------------------------------

void RTF::Reader::popState()
{
	if (m_states.isEmpty()) {
		return;
	}
	m_state = m_states.pop();
	m_cursor.setCharFormat(m_state.char_format);
	setFont(m_state.active_codepage);
}

//-----------------------------------------------------------------------------

void RTF::Reader::resetBlockFormatting(qint32)
{
	m_state.block_format = m_block_format;
	m_cursor.setBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::resetTextFormatting(qint32)
{
	m_state.char_format = QTextCharFormat();
	m_cursor.setCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setBlockAlignment(qint32 value)
{
	m_state.block_format.setAlignment(Qt::Alignment(value));
	m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setBlockDirection(qint32 value)
{
	m_state.block_format.setLayoutDirection(Qt::LayoutDirection(value));
	Qt::Alignment alignment = m_state.block_format.alignment();
	if (alignment & Qt::AlignLeft) {
		alignment |= Qt::AlignAbsolute;
		m_state.block_format.setAlignment(alignment);
	}
	m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setBlockIndent(qint32 value)
{
	m_state.block_format.setIndent(value / 15);
	m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextBold(qint32 value)
{
	m_state.char_format.setFontWeight(value ? QFont::Bold : QFont::Normal);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextItalic(qint32 value)
{
	m_state.char_format.setFontItalic(value);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextStrikeOut(qint32 value)
{
	m_state.char_format.setFontStrikeOut(value);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextUnderline(qint32 value)
{
	m_state.char_format.setFontUnderline(value);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setTextVerticalAlignment(qint32 value)
{
	m_state.char_format.setVerticalAlignment(QTextCharFormat::VerticalAlignment(value));
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RTF::Reader::setSkipCharacters(qint32 value)
{
	m_state.skip = value;
}

//-----------------------------------------------------------------------------

void RTF::Reader::setCodepage(qint32 value)
{
	QByteArray codepage;
	QTextCodec* codec = codecForCodePage(value, &codepage);
	if (codec != 0) {
		m_codepage = codec;
		m_codepage_name = codepage;
		setCodec(codec);
	}
}

//-----------------------------------------------------------------------------

void RTF::Reader::setFont(qint32 value)
{
	m_state.active_codepage = value;

	if (value < m_codepages.count()) {
		setCodec(m_codepages[value]);
	} else {
		setCodec(0);
		m_codepages.resize(value + 1);
	}

	if (m_codec == 0) {
		setCodec(m_codepage);
	}
}

//-----------------------------------------------------------------------------

void RTF::Reader::setFontCodepage(qint32 value)
{
	if (m_state.active_codepage >= m_codepages.count()) {
		m_state.ignore_control_word = true;
		m_state.ignore_text = true;
		return;
	}

	QTextCodec* codec = codecForCodePage(value);
	if (codec != 0) {
		m_codepages[m_state.active_codepage] = codec;
		setCodec(codec);
	}
	m_state.ignore_control_word = true;
	m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::setFontCharset(qint32 value)
{
	if (m_state.active_codepage >= m_codepages.count()) {
		m_state.ignore_text = true;
		return;
	}

	if (m_codepages[m_state.active_codepage] != 0) {
		setCodec(m_codepages[m_state.active_codepage]);
		m_state.ignore_text = true;
		return;
	}

	QByteArray charset;
	switch (value) {
	case 0: charset = "CP1252"; break;
	case 1: charset = "CP1252"; break;
	case 77: charset = "Apple Roman"; break;
	case 128: charset = "Shift-JIS"; break;
	case 129: charset = "eucKR"; break;
	case 130: charset = "CP1361"; break;
	case 134: charset = "GB2312"; break;
	case 136: charset = "Big5-HKSCS"; break;
	case 161: charset = "CP1253"; break;
	case 162: charset = "CP1254"; break;
	case 163: charset = "CP1258"; break;
	case 177: charset = "CP1255"; break;
	case 178: charset = "CP1256"; break;
	case 186: charset = "CP1257"; break;
	case 204: charset = "CP1251"; break;
	case 222: charset = "CP874"; break;
	case 238: charset = "CP1250"; break;
	case 255: charset = "CP850"; break;
	default: return;
	}

	QTextCodec* codec = QTextCodec::codecForName(charset);
	if (codec != 0) {
		m_codepages[m_state.active_codepage] = codec;
		setCodec(codec);
	}
	m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RTF::Reader::setCodec(QTextCodec* codec)
{
	if (m_codec != codec) {
		m_codec = codec;
		if (m_codec) {
			delete m_decoder;
			m_decoder = m_codec->makeDecoder();
		}
	}
}

//-----------------------------------------------------------------------------
