/*
	SPDX-FileCopyrightText: 2011-2015 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_ODT_READER_H
#define FOCUSWRITER_ODT_READER_H

#include "format_reader.h"

#include <QCoreApplication>
#include <QStack>
#include <QXmlStreamReader>

class OdtReader : public FormatReader
{
	Q_DECLARE_TR_FUNCTIONS(OdtReader)

public:
	explicit OdtReader();

	enum { Type = 3 };
	int type() const override
	{
		return Type;
	}

	static bool canRead(QIODevice* device);

private:
	void readData(QIODevice* device) override;
	void readDataCompressed(QIODevice* device);
	void readDataUncompressed(QIODevice* device);
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
			: block_format(block_format_)
			, char_format(char_format_)
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

#endif // FOCUSWRITER_ODT_READER_H
