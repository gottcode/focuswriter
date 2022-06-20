/*
	SPDX-FileCopyrightText: 2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "text_codec.h"

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringDecoder>
#include <QStringEncoder>

#ifdef HAVE_WINDOWS_ICU
#  include <icu.h>
#else
#  include <unicode/ucnv.h>
#endif

//-----------------------------------------------------------------------------

namespace
{

class TextCodecIcu : public TextCodec
{
public:
	explicit TextCodecIcu(const QByteArray& name);
	~TextCodecIcu();

	bool isValid() const
	{
		return m_converter;
	}

	QByteArray fromUnicode(const QString& input) override;
	QString toUnicode(const QByteArray& input) override;

private:
	UConverter* m_converter;
};

//-----------------------------------------------------------------------------

TextCodecIcu::TextCodecIcu(const QByteArray& name)
{
	UErrorCode error = U_ZERO_ERROR;

	m_converter = ucnv_open(name.constData(), &error);
	if (U_FAILURE(error)) {
		qDebug("TextCodec::TextCodec('%s') failed: %s", name.constData(), u_errorName(error));
		return;
	}

	ucnv_setSubstString(m_converter, u"?", 1, &error);
	if (U_SUCCESS(error)) {
		ucnv_setFromUCallBack(m_converter, UCNV_FROM_U_CALLBACK_SUBSTITUTE, nullptr, nullptr, nullptr, &error);
	}
	if (U_FAILURE(error)) {
		qDebug("TextCodec::TextCodec('%s') failed: %s", name.constData(), u_errorName(error));
	}
}

//-----------------------------------------------------------------------------

TextCodecIcu::~TextCodecIcu()
{
	if (isValid()) {
		ucnv_close(m_converter);
	}
}

//-----------------------------------------------------------------------------

QByteArray TextCodecIcu::fromUnicode(const QString& input)
{
	const UChar* source = reinterpret_cast<const UChar*>(input.constData());
	const UChar* source_limit = source + input.length();

	QByteArray output(UCNV_GET_MAX_BYTES_FOR_STRING(input.length(), ucnv_getMaxCharSize(m_converter)), Qt::Uninitialized);
	qsizetype converted_length = 0;

	UErrorCode error = U_ZERO_ERROR;
	do {
		// Resize output if it was too small
		if (converted_length) {
			output.resize(output.length() * 2);
		}

		// Set target past anything in output so far
		char* target = output.data();
		char* target_limit = target + output.length();
		target += converted_length;

		// Convert from Unicode
		error = U_ZERO_ERROR;
		ucnv_fromUnicode(m_converter, &target, target_limit, &source, source_limit, nullptr, false, &error);
		converted_length = target - output.data();
	} while (error == U_BUFFER_OVERFLOW_ERROR);

	// Shrink output to converted contents
	output.resize(converted_length);

	if (U_FAILURE(error)) {
		qDebug("TextCodec::fromUnicode() failed: %s", u_errorName(error));
	}

	return output;
}

//-----------------------------------------------------------------------------

QString TextCodecIcu::toUnicode(const QByteArray& input)
{
	const char* source = input.constData();
	const char* source_limit = source + input.length();

	QString output(input.length(), Qt::Uninitialized);
	qsizetype converted_length = 0;

	UErrorCode error = U_ZERO_ERROR;
	do {
		// Resize output if it was too small
		if (converted_length) {
			output.resize(output.length() * 2);
		}

		// Set target past anything in output so far
		UChar* target = reinterpret_cast<UChar*>(output.data());
		UChar* target_limit = target + output.length();
		target += converted_length;

		// Convert to Unicode
		error = U_ZERO_ERROR;
		ucnv_toUnicode(m_converter, &target, target_limit, &source, source_limit, nullptr, false, &error);
		converted_length = target - reinterpret_cast<UChar*>(output.data());
	} while (error == U_BUFFER_OVERFLOW_ERROR);

	// Shrink output to converted contents
	output.resize(converted_length);

	if (U_FAILURE(error)) {
		qDebug("TextCodec::toUnicode() failed: %s", u_errorName(error));
	}

	return output;
}

//-----------------------------------------------------------------------------

class TextCodecQt : public TextCodec
{
public:
	explicit TextCodecQt(QStringConverter::Encoding encoding)
		: m_decoder(encoding)
		, m_encoder(encoding)
	{
	}

	QByteArray fromUnicode(const QString& input) override
	{
		return m_encoder.encode(input);
	}

	QString toUnicode(const QByteArray& input) override
	{
		return m_decoder.decode(input);
	}

private:
	QStringDecoder m_decoder;
	QStringEncoder m_encoder;
};

//-----------------------------------------------------------------------------

class TextCodecCache
{
public:
	~TextCodecCache()
	{
		qDeleteAll(m_codecs);
	}

	TextCodec* fetch(const QByteArray& name)
	{
		// Check if name is alias of already loaded codec
		if (!m_codecs.contains(name)) {
			for (auto i = m_codecs.constBegin(), end = m_codecs.constEnd(); i != end; ++i) {
				if (ucnv_compareNames(name, i.key()) == 0) {
					m_codecs.insert(name, i.value());
					break;
				}
			}
		}

		// Create codec if not loaded yet
		if (!m_codecs.contains(name)) {
			const auto encoding = QStringConverter::encodingForName(name);
			if (encoding) {
				m_codecs.insert(name, new TextCodecQt(*encoding));
			} else {
				TextCodecIcu* codec = new TextCodecIcu(name);
				if (codec->isValid()) {
					m_codecs.insert(name, codec);
				} else {
					delete codec;
				}
			}
		}

		return m_codecs.value(name);
	}

private:
	QHash<QByteArray, TextCodec*> m_codecs;
};

}

//-----------------------------------------------------------------------------

TextCodec* TextCodec::codecForName(const QByteArray& name)
{
	static TextCodecCache codecs;
	return codecs.fetch(name);
}

//-----------------------------------------------------------------------------
