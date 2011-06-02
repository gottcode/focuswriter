/***********************************************************************
 *
 * Copyright (C) 2011 Graeme Gott <graeme@gottcode.org>
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

#include "converter.h"

#include "reader.h"
#include "writer.h"

#include <QBuffer>
#include <QTextDocument>

//-----------------------------------------------------------------------------

bool RTF::Converter::canConvert(const QString &mime, QString flavor)
{
	return flavorFor(mime) == flavor;
}

//-----------------------------------------------------------------------------

QList<QByteArray> RTF::Converter::convertFromMime(const QString &mime, QVariant data, QString flavor)
{
	QList<QByteArray> result;
	if (!canConvert(mime, flavor)) {
		return result;
	}

	// Parse HTML
	QTextDocument document;
	document.setHtml(data.toString());

	// Convert to RTF
	QByteArray rtf;
	QBuffer buffer(&rtf);
	buffer.open(QIODevice::WriteOnly);
	RTF::Writer writer;
	writer.write(&buffer, &document);

	result.append(rtf);
	return result;
}

//-----------------------------------------------------------------------------

QVariant RTF::Converter::convertToMime(const QString &mime, QList<QByteArray> data, QString flavor)
{
	if (!canConvert(mime, flavor)) {
		return QVariant();
	}

	// Parse RTF
	RTF::Reader reader;
	QTextDocument document;
	int count = data.count();
	for (int i = 0; i < count; ++i) {
		QBuffer buffer(&data[i]);
		buffer.open(QIODevice::ReadOnly);
		reader.read(&buffer, &document);
	}

	// Convert to HTML
	return document.toHtml();
}

//-----------------------------------------------------------------------------

QString RTF::Converter::convertorName()
{
	return QLatin1String("RichText");
}

//-----------------------------------------------------------------------------

QString RTF::Converter::flavorFor(const QString &mime)
{
	if (mime == QLatin1String("text/html")) {
		return QLatin1String("public.rtf");
	}
	return QString();
}

//-----------------------------------------------------------------------------

QString RTF::Converter::mimeFor(QString flavor)
{
	if (flavor == QLatin1String("public.rtf")) {
		return QLatin1String("text/html");
	}
	return QString();
}

//-----------------------------------------------------------------------------
