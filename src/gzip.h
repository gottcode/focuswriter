/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef GZIP_H
#define GZIP_H

class QByteArray;
class QString;

void gzip(const QString& path);
QByteArray gunzip(const QString& path);

#endif
