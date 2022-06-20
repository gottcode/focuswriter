/*
	SPDX-FileCopyrightText: 2010-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_RTF_READER_H
#define FOCUSWRITER_RTF_READER_H

#include "format_reader.h"
#include "rtf_tokenizer.h"
class TextCodec;

#include <QCoreApplication>
#include <QSet>
#include <QStack>
#include <QTextBlockFormat>
#include <QTextCharFormat>
class QString;

class RtfReader : public FormatReader
{
	Q_DECLARE_TR_FUNCTIONS(RtfReader)

public:
	explicit RtfReader();

	enum { Type = 2 };
	int type() const override
	{
		return Type;
	}

	static bool canRead(QIODevice* device);

private:
	void readData(QIODevice* device) override;
	void endBlock(qint32);
	void ignoreGroup(qint32);
	void ignoreText(qint32);
	void insertHexSymbol(qint32);
	void insertSymbol(qint32 value);
	void insertUnicodeSymbol(qint32 value);
	void insertText(const QString& text);
	void pushState();
	void popState();
	void resetBlockFormatting(qint32);
	void resetTextFormatting(qint32);
	void setBlockAlignment(qint32 value);
	void setBlockDirection(qint32 value);
	void setBlockIndent(qint32 value);
	void setTextBold(qint32 value);
	void setTextItalic(qint32 value);
	void setTextStrikeOut(qint32 value);
	void setTextUnderline(qint32 value);
	void setTextVerticalAlignment(qint32 value);
	void setSkipCharacters(qint32 value);
	void setCodepage(qint32 value);
	void setFont(qint32 value);
	void setFontCharset(qint32 value);
	void setFontCodepage(qint32 value);
	void setOutlineLevel(qint32 value);
	void setStyle(qint32 value);

	void startStyleSheet(qint32);
	void setStyleId(qint32 value);
	void setStyleParent(qint32 value);
	void setStyleName(const QString& style);
	void setStyleEnd();
	void setStyleSheetEnd();

private:
	RtfTokenizer m_token;
	bool m_in_block;

	class FunctionTable;

	struct Style
	{
		QTextCharFormat char_format;
		QTextBlockFormat block_format;
		const FunctionTable* functions;
		QSet<int> children;
	};
	QHash<int, Style> m_styles;

	struct State
	{
		QTextBlockFormat block_format;
		QTextCharFormat char_format;
		bool ignore_control_word;
		bool ignore_text;
		int skip;
		int active_codepage;
		const FunctionTable* functions;
		int style;
	};
	QStack<State> m_states;
	State m_state;
	QTextBlockFormat m_block_format;

	TextCodec* m_codec;
	TextCodec* m_codepage;
	QList<TextCodec*> m_codepages;
};

#endif // FOCUSWRITER_RTF_READER_H
