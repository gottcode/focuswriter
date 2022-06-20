/*
	SPDX-FileCopyrightText: 2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_TEXT_CODEC_H
#define FOCUSWRITER_TEXT_CODEC_H

class QByteArray;
class QString;

class TextCodec
{
public:
	TextCodec(const TextCodec&) = delete;
	TextCodec& operator=(const TextCodec&) = delete;

	virtual ~TextCodec()
	{
	}

	virtual QByteArray fromUnicode(const QString& input) = 0;
	virtual QString toUnicode(const QByteArray& input) = 0;

	static TextCodec* codecForName(const QByteArray& name);

protected:
	TextCodec()
	{
	}
};

#endif // FOCUSWRITER_TEXT_CODEC_H
