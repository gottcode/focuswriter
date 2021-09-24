/*
	SPDX-FileCopyrightText: 2011-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef RTF_CLIPBOARD_WINDOWS_H
#define RTF_CLIPBOARD_WINDOWS_H

#include <QWinMime>

namespace RTF
{
	class Clipboard : public QWinMime
	{
	public:
		Clipboard();

		virtual bool canConvertFromMime(const FORMATETC& format, const QMimeData* mime_data) const;
		virtual bool canConvertToMime(const QString& mime_type, IDataObject* data_obj) const;
		virtual bool convertFromMime(const FORMATETC& format, const QMimeData* mime_data, STGMEDIUM* storage_medium) const;
		virtual QVariant convertToMime(const QString& mime, IDataObject* data_obj, QVariant::Type preferred_type) const;
		virtual QVector<FORMATETC> formatsForMime(const QString& mime_type, const QMimeData* mime_data) const;
		virtual QString mimeForFormat(const FORMATETC& format) const;

	private:
		FORMATETC initFormat() const;

	private:
		int CF_RTF;
	};
}

#endif
