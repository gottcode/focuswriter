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

#include "clipboard_mac.h"

#include <QBuffer>

//-----------------------------------------------------------------------------

bool RTF::Clipboard::canConvert(const QString &mime, QString flavor)
{
	return flavorFor(mime) == flavor;
}

//-----------------------------------------------------------------------------

QList<QByteArray> RTF::Clipboard::convertFromMime(const QString &mime, QVariant data, QString flavor)
{
	QList<QByteArray> result;
	if (!canConvert(mime, flavor)) {
		return result;
	}

	result += data.toByteArray();
	return result;
}

//-----------------------------------------------------------------------------

QVariant RTF::Clipboard::convertToMime(const QString &mime, QList<QByteArray> data, QString flavor)
{
	if (!canConvert(mime, flavor)) {
		return QVariant();
	}

	QByteArray result;
	int count = data.count();
	for (int i = 0; i < count; ++i) {
		result += data[i];
	}
	return result;
}

//-----------------------------------------------------------------------------

QString RTF::Clipboard::convertorName()
{
	return QLatin1String("RichText");
}

//-----------------------------------------------------------------------------

QString RTF::Clipboard::flavorFor(const QString &mime)
{
	if (mime == QLatin1String("text/rtf")) {
		return QLatin1String("public.rtf");
	}
	return QString();
}

//-----------------------------------------------------------------------------

QString RTF::Clipboard::mimeFor(QString flavor)
{
	if (flavor == QLatin1String("public.rtf")) {
		return QLatin1String("text/rtf");
	}
	return QString();
}

//-----------------------------------------------------------------------------
