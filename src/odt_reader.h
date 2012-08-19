/***********************************************************************
 *
 * Copyright (C) 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include <QCoreApplication>
#include <QStack>
#include <QTextCursor>
#include <QXmlStreamReader>
class QIODevice;
class QTextDocument;

namespace ODT
{

class Reader
{
	Q_DECLARE_TR_FUNCTIONS(Reader)

public:
	Reader();

	QString errorString() const;
	bool hasError() const;

	void read(const QString& filename, QTextDocument* text);

private:
	void readDocument();
	void readStylesGroup();
	void readStyle();
	void readStyleParagraphProperties(QTextBlockFormat& format);
	void readStyleTextProperties(QTextCharFormat& format);
	void readBody();
	void readBodyText();
	void readParagraph();
	void readSpan();
	void readText();

private:
	QString m_filename;
	QXmlStreamReader m_xml;
	QTextCursor m_cursor;

	struct Style
	{
		Style(const QTextBlockFormat& block_format_ = QTextBlockFormat(), const QTextCharFormat& char_format_ = QTextCharFormat())
			: block_format(block_format_), char_format(char_format_)
		{
		}

		QTextBlockFormat block_format;
		QTextCharFormat char_format;
	};
	QHash<QString, Style> m_styles[2];
	QTextBlockFormat m_block_format;

	bool m_in_block;

	QString m_error;
};

}

#endif
