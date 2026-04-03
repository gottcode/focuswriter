/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "md_reader.h"

#include <QTextDocument>
#include <QTextStream>

//-----------------------------------------------------------------------------

MdReader::MdReader()
{
}

//-----------------------------------------------------------------------------

void MdReader::readData(QIODevice* device)
{
	QTextStream stream(device);
	m_cursor.document()->setMarkdown(stream.readAll(), QTextDocument::MarkdownDialectGitHub);
}

//-----------------------------------------------------------------------------
