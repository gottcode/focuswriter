/*
	SPDX-FileCopyrightText: 2010-2022 Graeme Gott <graeme@gottcode.org>
	SPDX-FileCopyrightText: 2001 Ewald Snel <ewald@rambo.its.tudelft.nl>
	SPDX-FileCopyrightText: 2001 Tomasz Grobelny <grotk@poczta.onet.pl>
	SPDX-FileCopyrightText: 2003, 2004 Nicolas GOUTTE <goutte@kde.org>

	SPDX-License-Identifier: GPL-3.0-or-later

	Derived in part from KWord's rtfimport.cpp
*/

#include "rtf_reader.h"

#include "text_codec.h"

#include <QFile>
#include <QTextBlock>

#include <cmath>

//-----------------------------------------------------------------------------

namespace
{

TextCodec* codecForCodePage(qint32 value)
{
	TextCodec* codec = nullptr;

	// Look up by ISO codec
	switch (value) {
	case   819: codec = TextCodec::codecForName("ISO-8859-1"); break;
	case  1200: codec = TextCodec::codecForName("UTF-16LE"); break;
	case  1201: codec = TextCodec::codecForName("UTF-16BE"); break;
	case 12000: codec = TextCodec::codecForName("UTF-32LE"); break;
	case 12001: codec = TextCodec::codecForName("UTF-32BE"); break;
	case 65001: codec = TextCodec::codecForName("UTF-8"); break;
	}
	if (codec) {
		return codec;
	}

	// Look up by codepage number
	codec = TextCodec::codecForName("CP" + QByteArray::number(value));
	if (codec) {
		return codec;
	}

	// Look up by fallback codec name
	switch (value) {
	case   708: codec = TextCodec::codecForName("ASMO-708"); break;
	case 10000: codec = TextCodec::codecForName("MACINTOSH"); break;
	case 10001: codec = TextCodec::codecForName("SJIS"); break;
	case 10002: codec = TextCodec::codecForName("BIG5"); break;
	case 10003: codec = TextCodec::codecForName("JOHAB"); break;
	case 10004: codec = TextCodec::codecForName("MACARABIC"); break;
	case 10005: codec = TextCodec::codecForName("MACHEBREW"); break;
	case 10006: codec = TextCodec::codecForName("MACGREEK"); break;
	case 10007: codec = TextCodec::codecForName("MACCYRILLIC"); break;
	case 10008: codec = TextCodec::codecForName("GB2312"); break;
	case 10010: codec = TextCodec::codecForName("MACROMANIA"); break;
	case 10017: codec = TextCodec::codecForName("MACUKRAINE"); break;
	case 10021: codec = TextCodec::codecForName("MACTHAI"); break;
	case 10029: codec = TextCodec::codecForName("MACCENTRALEUROPE"); break;
	case 10079: codec = TextCodec::codecForName("MACICELAND"); break;
	case 10081: codec = TextCodec::codecForName("MACTURKISH"); break;
	case 10082: codec = TextCodec::codecForName("MACCROATIAN"); break;
	}

	return codec;
}

}

//-----------------------------------------------------------------------------

class RtfReader::FunctionTable
{
public:
	explicit FunctionTable()
		: m_group_end_func(nullptr)
		, m_insert_text_func(nullptr)
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
		explicit Function(void (RtfReader::*func)(qint32) = nullptr, qint32 value = 0)
			: m_func(func)
			, m_value(value)
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

RtfReader::RtfReader()
	: m_in_block(true)
	, m_codec(nullptr)
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
					m_state.functions->insertText(this, m_codec->toUnicode(m_token.text()));
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
	m_cursor.insertText(m_codec->toUnicode(m_token.hex()));
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
	if (value > 0) {
		m_cursor.insertText(QChar(value));
	} else if (value < 0) {
		m_cursor.insertText(QChar(value + 0x10000));
	}

	for (int i = m_state.skip; i > 0;) {
		m_token.readNext();

		if (m_token.type() == TextToken) {
			const int len = m_token.text().length();
			if (len > i) {
				m_cursor.insertText(m_codec->toUnicode(m_token.text().mid(i)));
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
	TextCodec* codec = codecForCodePage(value);
	if (codec) {
		m_codepage = codec;
		m_codec = codec;
	}
}

//-----------------------------------------------------------------------------

void RtfReader::setFont(qint32 value)
{
	m_state.active_codepage = value;

	if (value < m_codepages.count()) {
		m_codec = m_codepages[value];
	} else {
		m_codec = nullptr;
		m_codepages.resize(value + 1);
	}

	if (!m_codec) {
		m_codec = m_codepage;
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

	TextCodec* codec = codecForCodePage(value);
	if (codec) {
		m_codepages[m_state.active_codepage] = codec;
		m_codec = codec;
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

	if (m_codepages[m_state.active_codepage]) {
		m_codec = m_codepages[m_state.active_codepage];
		m_state.ignore_text = true;
		return;
	}

	qint32 charset;
	switch (value) {
	case   0: charset = 1252; break;
	case   1: charset = 1252; break;
	case  77: charset = 10000; break;
	case  78: charset = 10001; break;
	case  79: charset = 10003; break;
	case  80: charset = 10008; break;
	case  81: charset = 10002; break;
	case  83: charset = 10005; break;
	case  84: charset = 10004; break;
	case  85: charset = 10006; break;
	case  86: charset = 10081; break;
	case  87: charset = 10021; break;
	case  88: charset = 10029; break;
	case  89: charset = 10007; break;
	case 128: charset = 932; break;
	case 129: charset = 949; break;
	case 130: charset = 1361; break;
	case 134: charset = 936; break;
	case 136: charset = 950; break;
	case 161: charset = 1253; break;
	case 162: charset = 1254; break;
	case 163: charset = 1258; break;
	case 177: charset = 1255; break;
	case 178: charset = 1256; break;
	case 186: charset = 1257; break;
	case 204: charset = 1251; break;
	case 222: charset = 874; break;
	case 238: charset = 1250; break;
	case 254: charset = 437; break;
	case 255: charset = 850; break;
	default: return;
	}

	TextCodec* codec = codecForCodePage(charset);
	if (codec) {
		m_codepages[m_state.active_codepage] = codec;
		m_codec = codec;
	}
	m_state.ignore_text = true;
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

	QHash<int, Style>::const_iterator style = m_styles.constFind(m_state.style);
	if (style != m_styles.constEnd()) {
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
	stylesheet_functions.setGroupEnd(nullptr);
}

//-----------------------------------------------------------------------------
