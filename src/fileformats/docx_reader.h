/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef DOCX_READER_H
#define DOCX_READER_H

#include "format_reader.h"

#include <QCoreApplication>
#include <QHash>
#include <QStack>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QXmlStreamReader>

class DocxReader : public FormatReader
{
	Q_DECLARE_TR_FUNCTIONS(DocxReader)

	struct Style
	{
		enum Type {
			Invalid = 0,
			Paragraph,
			Character
		} type;
		QTextBlockFormat block_format;
		QTextCharFormat char_format;

		void merge(const Style& style)
		{
			block_format.merge(style.block_format);
			char_format.merge(style.char_format);
		}
	};

public:
	DocxReader();

	enum { Type = 4 };
	int type() const
	{
		return Type;
	}

	static bool canRead(QIODevice* device);

private:
	void readData(QIODevice* device);
	void readContent();
	void readStyles();
	void readDocument();
	void readBody();
	void readParagraph();
	void readParagraphProperties(Style& style, bool allowstyles = true);
	void readRun();
	void readRunProperties(Style& style, bool allowstyles = true);
	void readText();

private:
	QXmlStreamReader m_xml;

	QHash<QString, Style> m_styles;
	QStack<Style> m_previous_styles;
	Style m_current_style;

	bool m_in_block;
};

#endif
