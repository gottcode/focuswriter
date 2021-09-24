/*
	SPDX-FileCopyrightText: 2013-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "txt_reader.h"

#include <QCoreApplication>
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#include <QTextCodec>
#endif
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
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
	m_encoding = QStringConverter::nameForEncoding(stream.encoding());
#else
	QTextCodec* codec = QTextCodec::codecForUtfText(device->peek(4), NULL);
	if (codec != NULL) {
		m_encoding = codec->name().toUpper();
	} else {
		codec = QTextCodec::codecForName("UTF-8");
	}
	stream.setCodec(codec);
#endif

	while (!stream.atEnd()) {
		m_cursor.insertText(stream.read(0x4000));
		QCoreApplication::processEvents();
	}

	m_cursor.endEditBlock();
}

//-----------------------------------------------------------------------------
