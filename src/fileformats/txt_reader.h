/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_TXT_READER_H
#define FOCUSWRITER_TXT_READER_H

#include "format_reader.h"

class TxtReader : public FormatReader
{
public:
	explicit TxtReader();

	enum { Type = 1 };
	int type() const override
	{
		return Type;
	}

	static bool canRead(QIODevice* device)
	{
		Q_UNUSED(device)
		return true;
	}

private:
	void readData(QIODevice* device) override;
};

#endif // FOCUSWRITER_TXT_READER_H
