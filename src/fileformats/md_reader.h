/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_MD_READER_H
#define FOCUSWRITER_MD_READER_H

#include "format_reader.h"

class MdReader : public FormatReader
{
public:
	explicit MdReader();

	enum { Type = 5 };
	int type() const override
	{
		return Type;
	}

private:
	void readData(QIODevice* device) override;
};

#endif // FOCUSWRITER_MD_READER_H
