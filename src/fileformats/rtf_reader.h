/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#ifndef RTF_READER_H
#define RTF_READER_H

#include "format_reader.h"
#include "rtf_tokenizer.h"

#include <QCoreApplication>
#include <QSet>
#include <QStack>
#include <QTextBlockFormat>
#include <QTextCharFormat>
class QString;
class QTextDecoder;

class RtfReader : public FormatReader
{
	Q_DECLARE_TR_FUNCTIONS(RtfReader)

public:
	RtfReader();
	~RtfReader();

	enum { Type = 2 };
	int type() const
	{
		return Type;
	}

	static bool canRead(QIODevice* device);

private:
	void readData(QIODevice* device);
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
	void setCodec(QTextCodec* codec);
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

	QTextCodec* m_codec;
	QTextDecoder* m_decoder;
	QTextCodec* m_codepage;
	QVector<QTextCodec*> m_codepages;
};

#endif
