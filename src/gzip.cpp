/***********************************************************************
 *
 * Copyright (C) 2011, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "gzip.h"

#include <QByteArray>
#include <QFile>

#include <zlib.h>

#include <algorithm>

//-----------------------------------------------------------------------------

void gzip(const QString& path)
{
	QFile file(path);
	if (!file.open(QFile::ReadOnly)) {
		return;
	}
	QByteArray data = file.readAll();
	file.close();

	if (!file.open(QFile::WriteOnly)) {
		return;
	}
	gzFile gz = gzdopen(file.handle(), "wb9");
	if (gz == NULL) {
		return;
	}

	gzwrite(gz, data.constData(), data.size());
	gzclose(gz);
}

//-----------------------------------------------------------------------------

QByteArray gunzip(const QString& path)
{
	QByteArray data;

	QFile file(path);
	if (!file.open(QFile::ReadOnly)) {
		return data;
	}
	gzFile gz = gzdopen(file.handle(), "rb");
	if (gz == NULL) {
		return data;
	}

	static const int buffer_size = 0x40000;
	char buffer[buffer_size];
	memset(buffer, 0, buffer_size);
	int read = 0;
	do {
		data.append(buffer, read);
		read = std::min(gzread(gz, buffer, buffer_size), buffer_size);
	} while (read > 0);
	gzclose(gz);

	return data;
}

//-----------------------------------------------------------------------------
