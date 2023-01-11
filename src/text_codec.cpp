/*
	SPDX-FileCopyrightText: 2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "text_codec.h"

#include <QByteArray>
#include <QHash>
#include <QString>

#include <iconv.h>

#include <errno.h>

//-----------------------------------------------------------------------------

namespace
{

class TextCodecIconv : public TextCodec
{
public:
	explicit TextCodecIconv(const QByteArray& name);
	~TextCodecIconv();

	bool isValid() const override
	{
		return m_from_cd != reinterpret_cast<iconv_t>(-1);
	}

	QByteArray fromUnicode(const QString& input) override;
	QString toUnicode(const QByteArray& input) override;

private:
	iconv_t m_from_cd;
	iconv_t m_to_cd;
};

//-----------------------------------------------------------------------------

TextCodecIconv::TextCodecIconv(const QByteArray& name)
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
	: TextCodec("UTF-16BE")
	, m_from_cd(iconv_open(name, "UTF-16BE"))
	, m_to_cd(iconv_open("UTF-16BE", name))
#else
	: TextCodec("UTF-16LE")
	, m_from_cd(iconv_open(name, "UTF-16LE"))
	, m_to_cd(iconv_open("UTF-16LE", name))
#endif
{
	if (m_from_cd == reinterpret_cast<iconv_t>(-1)) {
		qDebug("TextCodec::TextCodec('%s') failed", name.constData());
		if (m_to_cd != reinterpret_cast<iconv_t>(-1)) {
			iconv_close(m_to_cd);
			m_to_cd = reinterpret_cast<iconv_t>(-1);
		}
	} else if (m_to_cd == reinterpret_cast<iconv_t>(-1)) {
		qDebug("TextCodec::TextCodec('%s') failed", name.constData());
		iconv_close(m_from_cd);
		m_from_cd = reinterpret_cast<iconv_t>(-1);
	}
}

//-----------------------------------------------------------------------------

TextCodecIconv::~TextCodecIconv()
{
	if (TextCodecIconv::isValid()) {
		iconv_close(m_from_cd);
		iconv_close(m_to_cd);
	}
}

//-----------------------------------------------------------------------------

QByteArray TextCodecIconv::fromUnicode(const QString& input)
{
	QByteArray in = TextCodec::fromUnicode(input);
#ifndef __OS2__
	// POSIX requires the source to not be const, even though it does not modify it
	char* source = in.data();
#else
	const char* source = in.data();
#endif
	size_t source_remain = in.length();

	QByteArray output(input.length(), Qt::Uninitialized);
	char* dest = output.data();
	size_t dest_remain = output.length();

	while (source_remain) {
		if (iconv(m_from_cd, &source, &source_remain, &dest, &dest_remain) == static_cast<size_t>(-1)) {
			if (errno == E2BIG) {
				// Resize output buffer because it was too small
				const size_t converted_bytes = output.length() - dest_remain;
				output.resize(output.length() * 2);
				dest = output.data() + converted_bytes;
				dest_remain = output.length() - converted_bytes;
			} else if ((errno == EILSEQ) || (errno == EINVAL)) {
				// Skip invalid or incomplete multibyte sequence
				source += sizeof(QChar);
				source_remain -= sizeof(QChar);
			} else {
				// Abort on all other errors
				qDebug("TextCodec::fromUnicode() failed");

				// Reset converter to initial state
				iconv(m_from_cd, nullptr, &source_remain, nullptr, &dest_remain);

				return QByteArray();
			}
		}
	}

	// Shrink output to converted contents
	output.resize(output.length() - dest_remain);

	// Reset converter to initial state
	iconv(m_from_cd, nullptr, &source_remain, nullptr, &dest_remain);

	return output;
}

//-----------------------------------------------------------------------------

QString TextCodecIconv::toUnicode(const QByteArray& input)
{
#ifndef __OS2__
	// POSIX requires the source to not be const, even though it does not modify it
	char* source = const_cast<char*>(input.data());
#else
	const char* source = input.data();
#endif
	size_t source_remain = input.length();

	QByteArray output(input.length() * sizeof(QChar), Qt::Uninitialized);
	char* dest = output.data();
	size_t dest_remain = output.length();

	while (source_remain) {
		if (iconv(m_to_cd, &source, &source_remain, &dest, &dest_remain) == static_cast<size_t>(-1)) {
			if (errno == E2BIG) {
				// Resize output buffer because it was too small
				const size_t converted_bytes = output.length() - dest_remain;
				output.resize(output.length() * 2);
				dest = output.data() + converted_bytes;
				dest_remain = output.length() - converted_bytes;
			} else if ((errno == EILSEQ) || (errno == EINVAL)) {
				// Skip invalid or incomplete multibyte sequence
				++source;
				--source_remain;
			} else {
				// Abort on all other errors
				qDebug("TextCodec::toUnicode() failed");

				// Reset converter to initial state
				iconv(m_to_cd, nullptr, &source_remain, nullptr, &dest_remain);

				return QString();
			}
		}
	}

	// Shrink output to converted contents
	output.resize(output.length() - dest_remain);

	// Reset converter to initial state
	iconv(m_to_cd, nullptr, &source_remain, nullptr, &dest_remain);

	return TextCodec::toUnicode(output);
}

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
		TextCodec* codec = m_codecs.value(name, nullptr);
		if (codec) {
			return codec;
		}

		codec = new TextCodec(name);
		if (codec->isValid()) {
			m_codecs.insert(name, codec);
			return codec;
		} else {
			delete codec;
		}

		codec = new TextCodecIconv(name);
		if (codec->isValid()) {
			m_codecs.insert(name, codec);
			return codec;
		} else {
			delete codec;
		}

		return nullptr;
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
