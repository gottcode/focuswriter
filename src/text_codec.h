/*
	SPDX-FileCopyrightText: 2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_TEXT_CODEC_H
#define FOCUSWRITER_TEXT_CODEC_H

#include <QStringDecoder>
#include <QStringEncoder>

class TextCodec
{
public:
	explicit TextCodec(const QByteArray& encoding)
		: m_decoder(encoding)
		, m_encoder(encoding)
	{
	}

	TextCodec(const TextCodec&) = delete;
	TextCodec& operator=(const TextCodec&) = delete;

	virtual ~TextCodec()
	{
	}

	virtual bool isValid() const
	{
		return m_decoder.isValid() && m_encoder.isValid();
	}

	virtual QByteArray fromUnicode(const QString& input)
	{
		return m_encoder.encode(input);
	}

	virtual QString toUnicode(const QByteArray& input)
	{
		return m_decoder.decode(input);
	}

	static TextCodec* codecForName(const QByteArray& name);

private:
	QStringDecoder m_decoder;
	QStringEncoder m_encoder;
};

#endif // FOCUSWRITER_TEXT_CODEC_H
