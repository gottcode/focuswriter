/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DOCX_READER_H
#define FOCUSWRITER_DOCX_READER_H

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
	explicit DocxReader();

	enum { Type = 4 };
	int type() const override
	{
		return Type;
	}

	static bool canRead(QIODevice* device);

private:
	void readData(QIODevice* device) override;
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

#endif // FOCUSWRITER_DOCX_READER_H
