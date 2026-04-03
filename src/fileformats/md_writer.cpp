/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "md_writer.h"

#include <QTextDocument>
#include <QTextStream>

//-----------------------------------------------------------------------------

bool MdWriter::write(QIODevice* device, const QTextDocument* document)
{
	QTextStream stream(device);
	stream << document->toMarkdown(QTextDocument::MarkdownDialectGitHub);
	return stream.status() == QTextStream::Ok;
}

//-----------------------------------------------------------------------------
