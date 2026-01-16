/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "txt_reader.h"

#include <QTextStream>

//-----------------------------------------------------------------------------

TxtReader::TxtReader()
{
}

//-----------------------------------------------------------------------------

void TxtReader::readData(QIODevice* device)
{
	m_cursor.beginEditBlock();

	QTextStream stream(device);
	m_cursor.insertText(stream.readAll());

	m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------
