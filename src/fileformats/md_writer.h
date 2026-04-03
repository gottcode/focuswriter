/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_MD_WRITER_H
#define FOCUSWRITER_MD_WRITER_H

#include <QString>
class QIODevice;
class QTextDocument;

class MdWriter
{
public:
	QString errorString() const
	{
		return m_error;
	}

	bool write(QIODevice* device, const QTextDocument* document);

private:
	QString m_error;
};

#endif // FOCUSWRITER_MD_WRITER_H
