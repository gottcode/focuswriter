/*
	SPDX-FileCopyrightText: 2010-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef RTF_WRITER_H
#define RTF_WRITER_H

#include <QByteArray>
#include <QString>
class QIODevice;
class QTextCodec;
class QTextDocument;

class RtfWriter
{
public:
	RtfWriter(const QByteArray& encoding = QByteArray());

	QByteArray encoding() const;

	bool write(QIODevice* device, const QTextDocument* text);

private:
	void setCodec(QTextCodec* codec);
	QByteArray fromUnicode(const QString& string) const;

private:
	QByteArray m_encoding;
	QTextCodec* m_codec;
	bool m_supports_ascii;
	QByteArray m_header;
};

inline QByteArray RtfWriter::encoding() const
{
	return m_encoding;
}

#endif
