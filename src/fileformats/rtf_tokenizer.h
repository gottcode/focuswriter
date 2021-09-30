/*
	SPDX-FileCopyrightText: 2010-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_RTF_TOKENIZER_H
#define FOCUSWRITER_RTF_TOKENIZER_H

#include <QByteArray>
#include <QCoreApplication>
class QIODevice;

enum RtfTokenType
{
	StartGroupToken,
	EndGroupToken,
	ControlWordToken,
	TextToken
};

class RtfTokenizer
{
	Q_DECLARE_TR_FUNCTIONS(RtfTokenizer)

public:
	explicit RtfTokenizer();

	bool hasNext() const;
	bool hasValue() const;
	QByteArray hex() const;
	QByteArray text() const;
	RtfTokenType type() const;
	qint32 value() const;

	void readNext();
	void setDevice(QIODevice* device);

private:
	char next();

private:
	QIODevice* m_device;
	QByteArray m_buffer;
	int m_position;

	RtfTokenType m_type;
	QByteArray m_hex;
	QByteArray m_text;
	qint32 m_value;
	bool m_has_value;
};

inline bool RtfTokenizer::hasValue() const
{
	return m_has_value;
}

inline QByteArray RtfTokenizer::hex() const
{
	return m_hex;
}

inline QByteArray RtfTokenizer::text() const
{
	return m_text;
}

inline RtfTokenType RtfTokenizer::type() const
{
	return m_type;
}

inline qint32 RtfTokenizer::value() const
{
	return m_value;
}

#endif // FOCUSWRITER_RTF_TOKENIZER_H
