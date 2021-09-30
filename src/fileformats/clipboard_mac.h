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

		bool canConvert(const QString &mime, QString flavor) override;
		QList<QByteArray> convertFromMime(const QString &mime, QVariant data, QString flavor) override;
		QVariant convertToMime(const QString &mime, QList<QByteArray> data, QString flavor) override;
		QString convertorName() override;
		QString flavorFor(const QString &mime) override;
		QString mimeFor(QString flavor) override;
	};
}

#endif // FOCUSWRITER_RTF_CLIPBOARD_MAC_H
