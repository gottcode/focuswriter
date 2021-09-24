/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TXT_READER_H
#define TXT_READER_H

#include "format_reader.h"

class TxtReader : public FormatReader
{
public:
	TxtReader();

	enum { Type = 1 };
	int type() const
	{
		return Type;
	}

	static bool canRead(QIODevice* device)
	{
		Q_UNUSED(device)
		return true;
	}

private:
	void readData(QIODevice* device);
};

#endif
