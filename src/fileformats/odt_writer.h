/*
	SPDX-FileCopyrightText: 2013-2015 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_ODT_WRITER_H
#define FOCUSWRITER_ODT_WRITER_H

#include <QCoreApplication>
#include <QHash>
#include <QString>
#include <QXmlStreamWriter>
class QTextBlockFormat;
class QTextCharFormat;
class QTextDocument;

class OdtWriter
{
	Q_DECLARE_TR_FUNCTIONS(OdtWriter)

public:
	explicit OdtWriter();

	QString errorString() const
	{
		return m_error;
	}

	void setFlatXML(bool flat);

	bool write(QIODevice* device, const QTextDocument* document);

private:
	bool writeCompressed(QIODevice* device, const QTextDocument* document);
	bool writeUncompressed(QIODevice* device, const QTextDocument* document);
	QByteArray writeDocument(const QTextDocument* document);
	QByteArray writeStylesDocument(const QTextDocument* document);
	void writeStyles(const QTextDocument* document);
	void writeAutomaticStyles(const QTextDocument* document);
	bool writeParagraphStyle(const QTextBlockFormat& format, const QString& name);
	bool writeTextStyle(const QTextCharFormat& format, const QString& name);
	void writeBody(const QTextDocument* document);

private:
	QXmlStreamWriter m_xml;
	QHash<int, QString> m_styles;
	QString m_error;
	bool m_flat;
};

#endif // FOCUSWRITER_ODT_WRITER_H
