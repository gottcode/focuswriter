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
