/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#include <cmath>

//-----------------------------------------------------------------------------

namespace
{
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

class RtfReader::FunctionTable
{
public:
	FunctionTable() :
		m_group_end_func(0),
		m_insert_text_func(0)
	{
	}

	void call(RtfReader* reader, const RtfTokenizer& token) const
	{
		m_functions[token.text()].call(reader, token);
	}

	bool contains(const QByteArray& name) const
	{
		return m_functions.contains(name);
	}

	void groupEnd(RtfReader* reader) const
	{
		if (m_group_end_func) {
			(reader->*m_group_end_func)();
		}
	}

	void insertText(RtfReader* reader, const QString& text) const
	{
		(reader->*m_insert_text_func)(text);
	}

	bool isEmpty() const
	{
		return m_functions.isEmpty();
	}

	void set(const QByteArray& name, void (RtfReader::*func)(qint32), qint32 value = 0)
	{
		m_functions[name] = Function(func, value);
	}

	void setGroupEnd(void (RtfReader::*groupEndFunc)())
	{
		m_group_end_func = groupEndFunc;
	}

	void setInsertText(void (RtfReader::*insertTextFunc)(const QString&))
	{
		m_insert_text_func = insertTextFunc;
	}

	void unset(const QByteArray& name)
	{
		m_functions.remove(name);
	}

private:
	void (RtfReader::*m_group_end_func)();
	void (RtfReader::*m_insert_text_func)(const QString&);

	class Function
	{
	public:
		Function(void (RtfReader::*func)(qint32) = 0, qint32 value = 0) :
			m_func(func),
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
	QHash<QByteArray, Function> m_functions;
}
functions,
stylesheet_functions,
heading_functions;

//-----------------------------------------------------------------------------

RtfReader::RtfReader() :
	m_in_block(true),
	m_codec(0),
	m_decoder(0)
{
	if (functions.isEmpty()) {
		functions.setInsertText(&RtfReader::insertText);

		functions.set("\\", &RtfReader::insertSymbol, '\\');
		functions.set("_", &RtfReader::insertSymbol, 0x2011);
		functions.set("{", &RtfReader::insertSymbol, '{');
		functions.set("|", &RtfReader::insertSymbol, 0x00b7);
		functions.set("}", &RtfReader::insertSymbol, '}');
		functions.set("~", &RtfReader::insertSymbol, 0x00a0);
		functions.set("-", &RtfReader::insertSymbol, 0x00ad);

		functions.set("bullet", &RtfReader::insertSymbol, 0x2022);
		functions.set("emdash", &RtfReader::insertSymbol, 0x2014);
		functions.set("emspace", &RtfReader::insertSymbol, 0x2003);
		functions.set("endash", &RtfReader::insertSymbol, 0x2013);
		functions.set("enspace", &RtfReader::insertSymbol, 0x2002);
		functions.set("ldblquote", &RtfReader::insertSymbol, 0x201c);
		functions.set("lquote", &RtfReader::insertSymbol, 0x2018);
		functions.set("line", &RtfReader::insertSymbol, 0x2028);
		functions.set("ltrmark", &RtfReader::insertSymbol, 0x200e);
		functions.set("qmspace", &RtfReader::insertSymbol, 0x2004);
		functions.set("rdblquote", &RtfReader::insertSymbol, 0x201d);
		functions.set("rquote", &RtfReader::insertSymbol, 0x2019);
		functions.set("rtlmark", &RtfReader::insertSymbol, 0x200f);
		functions.set("tab", &RtfReader::insertSymbol, 0x0009);
		functions.set("zwj", &RtfReader::insertSymbol, 0x200d);
		functions.set("zwnj", &RtfReader::insertSymbol, 0x200c);

		functions.set("\'", &RtfReader::insertHexSymbol);
		functions.set("u", &RtfReader::insertUnicodeSymbol);
		functions.set("uc", &RtfReader::setSkipCharacters);
		functions.set("par", &RtfReader::endBlock);
		functions.set("\n", &RtfReader::endBlock);
		functions.set("\r", &RtfReader::endBlock);

		functions.set("pard", &RtfReader::resetBlockFormatting);
		functions.set("plain", &RtfReader::resetTextFormatting);

		functions.set("qc", &RtfReader::setBlockAlignment, Qt::AlignHCenter);
		functions.set("qj", &RtfReader::setBlockAlignment, Qt::AlignJustify);
		functions.set("ql", &RtfReader::setBlockAlignment, Qt::AlignLeft | Qt::AlignAbsolute);
		functions.set("qr", &RtfReader::setBlockAlignment, Qt::AlignRight | Qt::AlignAbsolute);

		functions.set("li", &RtfReader::setBlockIndent);

		functions.set("ltrpar", &RtfReader::setBlockDirection, Qt::LeftToRight);
		functions.set("rtlpar", &RtfReader::setBlockDirection, Qt::RightToLeft);

		functions.set("b", &RtfReader::setTextBold, true);
		functions.set("i", &RtfReader::setTextItalic, true);
		functions.set("strike", &RtfReader::setTextStrikeOut, true);
		functions.set("striked", &RtfReader::setTextStrikeOut, true);
		functions.set("ul", &RtfReader::setTextUnderline, true);
		functions.set("uld", &RtfReader::setTextUnderline, true);
		functions.set("uldash", &RtfReader::setTextUnderline, true);
		functions.set("uldashd", &RtfReader::setTextUnderline, true);
		functions.set("uldb", &RtfReader::setTextUnderline, true);
		functions.set("ulnone", &RtfReader::setTextUnderline, false);
		functions.set("ulth", &RtfReader::setTextUnderline, true);
		functions.set("ulw", &RtfReader::setTextUnderline, true);
		functions.set("ulwave", &RtfReader::setTextUnderline, true);
		functions.set("ulhwave", &RtfReader::setTextUnderline, true);
		functions.set("ululdbwave", &RtfReader::setTextUnderline, true);

		functions.set("sub", &RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignSubScript);
		functions.set("super", &RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignSuperScript);
		functions.set("nosupersub", &RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignNormal);

		functions.set("outlinelevel", &RtfReader::setOutlineLevel, 0);

		functions.set("ansicpg", &RtfReader::setCodepage);
		functions.set("ansi", &RtfReader::setCodepage, 1252);
		functions.set("mac", &RtfReader::setCodepage, 10000);
		functions.set("pc", &RtfReader::setCodepage, 850);
		functions.set("pca", &RtfReader::setCodepage, 850);

		functions.set("deff", &RtfReader::setFont);
		functions.set("f", &RtfReader::setFont);
		functions.set("cpg", &RtfReader::setFontCodepage);
		functions.set("fcharset", &RtfReader::setFontCharset);

		functions.set("stylesheet", &RtfReader::startStyleSheet);
		functions.set("s", &RtfReader::setStyle);

		functions.set("filetbl", &RtfReader::ignoreGroup);
		functions.set("colortbl", &RtfReader::ignoreGroup);
		functions.set("fonttbl", &RtfReader::ignoreText);
		functions.set("info", &RtfReader::ignoreGroup);
		functions.set("pict", &RtfReader::ignoreGroup);
		functions.set("*", &RtfReader::ignoreGroup);
	}

	if (stylesheet_functions.isEmpty()) {
		stylesheet_functions.set("s", &RtfReader::setStyleId, 0);
		stylesheet_functions.set("sbasedon", &RtfReader::setStyleParent, 0);
		stylesheet_functions.setInsertText(&RtfReader::setStyleName);

		stylesheet_functions.set("qc", &RtfReader::setBlockAlignment, Qt::AlignHCenter);
		stylesheet_functions.set("qj", &RtfReader::setBlockAlignment, Qt::AlignJustify);
		stylesheet_functions.set("ql", &RtfReader::setBlockAlignment, Qt::AlignLeft | Qt::AlignAbsolute);
		stylesheet_functions.set("qr", &RtfReader::setBlockAlignment, Qt::AlignRight | Qt::AlignAbsolute);

		stylesheet_functions.set("li", &RtfReader::setBlockIndent);

		stylesheet_functions.set("ltrpar", &RtfReader::setBlockDirection, Qt::LeftToRight);
		stylesheet_functions.set("rtlpar", &RtfReader::setBlockDirection, Qt::RightToLeft);

		stylesheet_functions.set("b", &RtfReader::setTextBold, true);
		stylesheet_functions.set("i", &RtfReader::setTextItalic, true);
		stylesheet_functions.set("strike", &RtfReader::setTextStrikeOut, true);
		stylesheet_functions.set("striked", &RtfReader::setTextStrikeOut, true);
		stylesheet_functions.set("ul", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("uld", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("uldash", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("uldashd", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("uldb", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("ulnone", &RtfReader::setTextUnderline, false);
		stylesheet_functions.set("ulth", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("ulw", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("ulwave", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("ulhwave", &RtfReader::setTextUnderline, true);
		stylesheet_functions.set("ululdbwave", &RtfReader::setTextUnderline, true);

		stylesheet_functions.set("sub", &RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignSubScript);
		stylesheet_functions.set("super", &RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignSuperScript);
		stylesheet_functions.set("nosupersub", &RtfReader::setTextVerticalAlignment, QTextCharFormat::AlignNormal);

		stylesheet_functions.set("outlinelevel", &RtfReader::setOutlineLevel, 0);
	}

	if (heading_functions.isEmpty()) {
		heading_functions = functions;
		heading_functions.unset("b");
	}

	m_state.ignore_control_word = false;
	m_state.ignore_text = false;
	m_state.skip = 1;
	m_state.active_codepage = 0;
	m_state.functions = &functions;
	m_state.style = 0;

	setCodepage(1252);
}

//-----------------------------------------------------------------------------

RtfReader::~RtfReader()
{
	delete m_decoder;
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
				m_state.functions->groupEnd(this);
				popState();
			} else if (m_token.type() == ControlWordToken) {
				if (!m_state.ignore_control_word && m_state.functions->contains(m_token.text())) {
					m_state.functions->call(this, m_token);
				}
			} else if (m_token.type() == TextToken) {
				if (!m_state.ignore_text) {
					m_state.functions->insertText(this, m_decoder->toUnicode(m_token.text()));
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

void RtfReader::insertText(const QString& text)
{
	m_cursor.insertText(text);
}

//-----------------------------------------------------------------------------

void RtfReader::insertUnicodeSymbol(qint32 value)
{
	if (value)
	{
		m_cursor.insertText(QChar(value));
	}

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
	m_state.block_format.setIndent(std::lround(static_cast<double>(value) / 720.0));
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
		m_encoding = codepage;
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

void RtfReader::setOutlineLevel(qint32 value)
{
	m_state.block_format.setProperty(QTextFormat::UserProperty, qBound(1, value + 1, 6));
	m_cursor.mergeBlockFormat(m_state.block_format);
}

//-----------------------------------------------------------------------------

void RtfReader::setStyle(qint32 value)
{
	m_state.style = value;

	QHash<int, Style>::const_iterator style = m_styles.find(m_state.style);
	if (style != m_styles.end()) {
		m_state.block_format.merge(style->block_format);
		m_cursor.mergeBlockFormat(m_state.block_format);

		m_state.char_format.merge(style->char_format);
		m_cursor.mergeCharFormat(m_state.char_format);

		m_state.functions = style->functions;
	}
}

//-----------------------------------------------------------------------------

void RtfReader::startStyleSheet(qint32)
{
	m_state.functions = &stylesheet_functions;
}

//-----------------------------------------------------------------------------

void RtfReader::setStyleId(qint32 value)
{
	m_state.style = value;
	stylesheet_functions.setGroupEnd(&RtfReader::setStyleEnd);
}

//-----------------------------------------------------------------------------

void RtfReader::setStyleParent(qint32 value)
{
	Style& style = m_styles[m_state.style];

	Style& parent = m_styles[value];

	QTextBlockFormat block_format = parent.block_format;
	block_format.merge(style.block_format);
	style.block_format = block_format;

	QTextCharFormat char_format = parent.char_format;
	char_format.merge(style.char_format);
	style.char_format = char_format;

	parent.children.insert(m_state.style);
}

//-----------------------------------------------------------------------------

void RtfReader::setStyleName(const QString& style)
{
	int heading = -1;
	if (style.startsWith("Head")) {
		heading = qBound(1, style.at(style.length() - 2).digitValue(), 6);
	}
	if (heading != -1) {
		m_styles[m_state.style].block_format.setProperty(QTextFormat::UserProperty, heading);
	}
}

//-----------------------------------------------------------------------------

void RtfReader::setStyleEnd()
{
	Style& style = m_styles[m_state.style];
	style.block_format.merge(m_state.block_format);
	style.char_format.merge(m_state.char_format);
	if (style.block_format.property(QTextFormat::UserProperty).toInt() != 0) {
		style.char_format.setFontWeight(QFont::Normal);
		style.functions = &heading_functions;
	} else {
		style.functions = &functions;
	}

	QSetIterator<int> iter(style.children);
	while (iter.hasNext()) {
		Style& child = m_styles[iter.next()];

		QTextBlockFormat block_format = style.block_format;
		block_format.merge(child.block_format);
		child.block_format = block_format;

		QTextCharFormat char_format = style.char_format;
		char_format.merge(child.block_format);
		child.char_format = char_format;
	}

	stylesheet_functions.setGroupEnd(&RtfReader::setStyleSheetEnd);
}

//-----------------------------------------------------------------------------

void RtfReader::setStyleSheetEnd()
{
	stylesheet_functions.setGroupEnd(0);
}

//-----------------------------------------------------------------------------
