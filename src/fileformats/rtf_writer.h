/*
	SPDX-FileCopyrightText: 2010-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_RTF_WRITER_H
#define FOCUSWRITER_RTF_WRITER_H

#include <QByteArray>
#include <QString>
class QIODevice;
class QTextDocument;

class RtfWriter
{
public:
	bool write(QIODevice* device, const QTextDocument* text);

private:
	QByteArray fromUnicode(const QString& string) const;
};

#endif // FOCUSWRITER_RTF_WRITER_H
