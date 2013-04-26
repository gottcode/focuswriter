/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "rtf_reader.h"

#include <QFile>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextDecoder>

//-----------------------------------------------------------------------------

namespace
{
	class RtfFunction
	{
	public:
		RtfFunction(void (RtfReader::*func)(qint32) = 0, qint32 value = 0)
			: m_func(func),
			m_value(value)
		{
		}

		void call(RtfReader* reader, const RtfTokenizer& token) const
		{
			(reader->*m_func)(token.hasValue() ? token.value() : m_value);
		}

	private:
		void (RtfReader::*m_func)(qint32);
		qint32 m_value;
	};
	QHash<QByteArray, RtfFunction> functions;

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

RtfReader::RtfReader() :
	m_in_block(true),
	m_codec(0),
	m_decoder(0)
{
	if (functions.isEmpty()) {
		functions["\\"] = RtfFunction(&RtfReader::insertSymbol, '\\');
		functions["_"] = RtfFunction(&RtfReader::insertSymbol, 0x2011);
		functions["{"] = RtfFunction(&RtfReader::insertSymbol, '{');
		functions["|"] = RtfFunction(&RtfReader::insertSymbol, 0x00b7);
		functions["}"] = RtfFunction(&RtfReader::insertSymbol, '}');
		functions["~"] = RtfFunction(&RtfReader::insertSymbol, 0x00a0);
		functions["-"] = RtfFunction(&RtfReader::insertSymbol, 0x00ad);

		functions["bullet"] = RtfFunction(&RtfReader::insertSymbol, 0x2022);
		functions["emdash"] = RtfFunction(&RtfReader::insertSymbol, 0x2014);
		functions["emspace"] = RtfFunction(&RtfReader::insertSymbol, 0x2003);
		functions["endash"] = RtfFunction(&RtfReader::insertSymbol, 0x2013);
		functions["enspace"] = RtfFunction(&RtfReader::insertSymbol, 0x2002);
		functions["ldblquote"] = RtfFunction(&RtfReader::insertSymbol, 0x201c);
		functions["lquote"] = RtfFunction(&RtfReader::insertSymbol, 0x2018);
		functions["line"] = RtfFunction(&RtfReader::insertSymbol, 0x2028);
		functions["ltrmark"] = RtfFunction(&RtfReader::insertSymbol, 0x200e);
		functions["qmspace"] = RtfFunction(&RtfReader::insertSymbol, 0x2004);
		functions["rdblquote"] = RtfFunction(&RtfReader::insertSymbol, 0x201d);
		functions["rquote"] = RtfFunction(&RtfReader::insertSymbol, 0x2019);
		functions["rtlmark"] = RtfFunction(&RtfReader::insertSymbol, 0x200f);
		functions["tab"] = RtfFunction(&RtfReader::insertSymbol, 0x0009);
		functions["zwj"] = RtfFunction(&RtfReader::insertSymbol, 0x200d);
		functions["zwnj"] = RtfFunction(&RtfReader::insertSymbol, 0x200c);

		functions["\'"] = RtfFunction(&RtfReader::insertHexSymbol);
		functions["u"] = RtfFunction(&RtfReader::insertUnicodeSymbol);
		functions["uc"] = RtfFunction(&RtfReader::setSkipCharacters);
		functions["par"] = RtfFunction(&RtfReader::endBlock);
		functions["\n"] = RtfFunction(&RtfReader::endBlock);
		functions["\r"] = RtfFunction(&RtfReader::endBlock);

		functions["pard"] = RtfFunction(&RtfReader::resetBlockFormatting);
		functions["plain"] = RtfFunction(&RtfReader::resetTextFormatting);

		functions["qc"] = RtfFunction(&RtfReader::setBlockAlignment, Qt::AlignHCenter);
		functions["qj"] = RtfFunction(&RtfReader::setBlockAlignment, Qt::AlignJustify);
		functions["ql"] = RtfFunction(&RtfReader::setBlockAlignment, Qt::AlignLeft | Qt::AlignAbsolute);
		functions["qr"] = RtfFunction(&RtfReader::setBlockAlignment, Qt::AlignRight | Qt::AlignAbsolute);

		functions["li"] = RtfFunction(&RtfReader::setBlockIndent);

		functions["ltrpar"] = RtfFunction(&RtfReader::setBlockDirection, Qt::LeftToRight);
		functions["rtlpar"] = RtfFunction(&RtfReader::setBlockDirection, Qt::RightToLeft);

		functions["b"] = RtfFunction(&RtfReader::setTextBold, true);
		functions["i"] = RtfFunction(&RtfReader::setTextItalic, true);
		functions["strike"] = RtfFunction(&RtfReader::setTextStrikeOut, true);
		functions["striked"] = RtfFunction(&RtfReader::setTextStrikeOut, true);
		functions["ul"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["uld"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["uldash"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["uldashd"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["uldb"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["ulnone"] = RtfFunction(&RtfReader::setTextUnderline, false);
		functions["ulth"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["ulw"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["ulwave"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["ulhwave"] = RtfFunction(&RtfReader::setTextUnderline, true);
		functions["ululdbwave"] = RtfFunction(&RtfReader::setTextUnderline, true);

		functions["sub"] = RtfFunction(&RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignSubScript);
		functions["super"] = RtfFunction(&RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignSuperScript);
		functions["nosupersub"] = RtfFunction(&RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignNormal);

		functions["ansicpg"] = RtfFunction(&RtfReader::setCodepage);
		functions["ansi"] = RtfFunction(&RtfReader::setCodepage, 1252);
		functions["mac"] = RtfFunction(&RtfReader::setCodepage, 10000);
		functions["pc"] = RtfFunction(&RtfReader::setCodepage, 850);
		functions["pca"] = RtfFunction(&RtfReader::setCodepage, 850);

		functions["deff"] = RtfFunction(&RtfReader::setFont);
		functions["f"] = RtfFunction(&RtfReader::setFont);
		functions["cpg"] = RtfFunction(&RtfReader::setFontCodepage);
		functions["fcharset"] = RtfFunction(&RtfReader::setFontCharset);

		functions["filetbl"] = RtfFunction(&RtfReader::ignoreGroup);
		functions["colortbl"] = RtfFunction(&RtfReader::ignoreGroup);
		functions["fonttbl"] = RtfFunction(&RtfReader::ignoreText);
		functions["stylesheet"] = RtfFunction(&RtfReader::ignoreGroup);
		functions["info"] = RtfFunction(&RtfReader::ignoreGroup);
		functions["*"] = RtfFunction(&RtfReader::ignoreGroup);
	}

	m_state.ignore_control_word = false;
	m_state.ignore_text = false;
	m_state.skip = 1;
	m_state.active_codepage = 0;

	setCodepage(1252);
}

//-----------------------------------------------------------------------------

RtfReader::~RtfReader()
{
	delete m_decoder;
}

//-----------------------------------------------------------------------------

QByteArray RtfReader::codePage() const
{
	return m_codepage_name;
}

//-----------------------------------------------------------------------------

bool RtfReader::canRead(QIODevice* device)
{
	return device->peek(5) == "{\\rtf";
}

//-----------------------------------------------------------------------------

void RtfReader::readData(QIODevice* device)
{
	try {
		// Use theme spacings
		m_block_format = m_cursor.blockFormat();
		m_state.block_format = m_block_format;

		// Open file
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

void RtfReader::endBlock(qint32)
{
	m_in_block = false;
}

//-----------------------------------------------------------------------------

void RtfReader::ignoreGroup(qint32)
{
	m_state.ignore_control_word = true;
	m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RtfReader::ignoreText(qint32)
{
	m_state.ignore_text = true;
}

//-----------------------------------------------------------------------------

void RtfReader::insertHexSymbol(qint32)
{
	m_cursor.insertText(m_decoder->toUnicode(m_token.hex()));
}

//-----------------------------------------------------------------------------

void RtfReader::insertSymbol(qint32 value)
{
	m_cursor.insertText(QChar(value));
}

//-----------------------------------------------------------------------------

void RtfReader::insertUnicodeSymbol(qint32 value)
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

void RtfReader::pushState()
{
	m_states.push(m_state);
}

//-----------------------------------------------------------------------------

void RtfReader::popState()
{
	if (m_states.isEmpty()) {
		return;
	}
	m_state = m_states.pop();
	m_cursor.setCharFormat(m_state.char_format);
	setFont(m_state.active_codepage);
}

//-----------------------------------------------------------------------------

void RtfReader::resetBlockFormatting(qint32)
{
	m_state.block_format = m_block_format;
	m_cursor.setBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RtfReader::resetTextFormatting(qint32)
{
	m_state.char_format = QTextCharFormat();
	m_cursor.setCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setBlockAlignment(qint32 value)
{
	m_state.block_format.setAlignment(Qt::Alignment(value));
	m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setBlockDirection(qint32 value)
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

void RtfReader::setBlockIndent(qint32 value)
{
	m_state.block_format.setIndent(value / 15);
	m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setTextBold(qint32 value)
{
	m_state.char_format.setFontWeight(value ? QFont::Bold : QFont::Normal);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setTextItalic(qint32 value)
{
	m_state.char_format.setFontItalic(value);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setTextStrikeOut(qint32 value)
{
	m_state.char_format.setFontStrikeOut(value);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setTextUnderline(qint32 value)
{
	m_state.char_format.setFontUnderline(value);
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setTextVerticalAlignment(qint32 value)
{
	m_state.char_format.setVerticalAlignment(QTextCharFormat::VerticalAlignment(value));
	m_cursor.mergeCharFormat(m_state.char_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setSkipCharacters(qint32 value)
{
	m_state.skip = value;
}

//-----------------------------------------------------------------------------

void RtfReader::setCodepage(qint32 value)
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

void RtfReader::setFont(qint32 value)
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

void RtfReader::setFontCodepage(qint32 value)
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

void RtfReader::setFontCharset(qint32 value)
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

void RtfReader::setCodec(QTextCodec* codec)
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
