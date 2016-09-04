/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2015 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#ifndef ODT_WRITER_H
#define ODT_WRITER_H

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
	OdtWriter();

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

#endif
