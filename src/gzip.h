/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_GZIP_H
#define FOCUSWRITER_GZIP_H

class QByteArray;
class QString;

void gzip(const QString& path);
QByteArray gunzip(const QString& path);

#endif // FOCUSWRITER_GZIP_H
