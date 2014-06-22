/***********************************************************************
 *
 * Copyright (C) 2011, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#ifndef ODT_READER_H
#define ODT_READER_H

#include "format_reader.h"

#include <QCoreApplication>
#include <QStack>
#include <QXmlStreamReader>

class OdtReader : public FormatReader
{
	Q_DECLARE_TR_FUNCTIONS(OdtReader)

public:
	OdtReader();

	enum { Type = 3 };
	int type() const
	{
		return Type;
	}

	static bool canRead(QIODevice* device);

private:
	void readData(QIODevice* device);
	void readDocument();
	void readStylesGroup();
	void readStyle();
	void readStyleParagraphProperties(QTextBlockFormat& format);
	void readStyleTextProperties(QTextCharFormat& format);
	void readBody();
	void readBodyText();
	void readParagraph(int level = 0);
	void readSpan();
	void readText();

private:
	QXmlStreamReader m_xml;

	struct Style
	{
		Style(const QTextBlockFormat& block_format_ = QTextBlockFormat(), const QTextCharFormat& char_format_ = QTextCharFormat())
			: block_format(block_format_), char_format(char_format_)
		{
		}

		QTextBlockFormat block_format;
		QTextCharFormat char_format;
		QStringList children;
	};
	QHash<QString, Style> m_styles[2];
	QTextBlockFormat m_block_format;

	bool m_in_block;
};

#endif
