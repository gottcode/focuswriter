/*
	SPDX-FileCopyrightText: 2011-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "gzip.h"

#include <QByteArray>
#include <QFile>

#include <zlib.h>

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
	if (!gz) {
		return;
	}

	gzwrite(gz, data.constData(), data.length());
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
	if (!gz) {
		return data;
	}

	QByteArray buffer(0x40000, 0);
	int read = 0;
	do {
		data.append(buffer.constData(), read);
		read = gzread(gz, buffer.data(), buffer.size());
	} while (read > 0);
	gzclose(gz);

	return data;
}

//-----------------------------------------------------------------------------
