/*
	SPDX-FileCopyrightText: 2013-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "txt_reader.h"

#include <QCoreApplication>
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

	while (!stream.atEnd()) {
		m_cursor.insertText(stream.read(0x4000));
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------
