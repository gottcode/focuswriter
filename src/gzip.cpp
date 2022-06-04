/*
	SPDX-FileCopyrightText: 2011-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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
	const QByteArray data = file.readAll();
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
