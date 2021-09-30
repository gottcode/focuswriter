/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_RTF_CLIPBOARD_MAC_H
#define FOCUSWRITER_RTF_CLIPBOARD_MAC_H

#include <QMacPasteboardMime>

namespace RTF
{
	class Clipboard : public QMacPasteboardMime
	{
	public:
		explicit Clipboard()
			: QMacPasteboardMime(MIME_ALL)
		{
		}

		virtual bool canConvert(const QString &mime, QString flavor);
		virtual QList<QByteArray> convertFromMime(const QString &mime, QVariant data, QString flavor);
		virtual QVariant convertToMime(const QString &mime, QList<QByteArray> data, QString flavor);
		virtual QString convertorName();
		virtual QString flavorFor(const QString &mime);
		virtual QString mimeFor(QString flavor);
	};
}

#endif // FOCUSWRITER_RTF_CLIPBOARD_MAC_H
