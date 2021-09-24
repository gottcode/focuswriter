/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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
