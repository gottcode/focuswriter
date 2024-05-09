/*
	SPDX-FileCopyrightText: 2011-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef RTF_CLIPBOARD_WINDOWS_H
#define RTF_CLIPBOARD_WINDOWS_H

#include <QWindowsMimeConverter>

class RtfClipboard : public QWindowsMimeConverter
{
public:
	RtfClipboard();

	bool canConvertFromMime(const FORMATETC& format, const QMimeData* mime_data) const override;
	bool canConvertToMime(const QString& mime_type, IDataObject* data_obj) const override;
	bool convertFromMime(const FORMATETC& format, const QMimeData* mime_data, STGMEDIUM* storage_medium) const override;
	QVariant convertToMime(const QString& mime, IDataObject* data_obj, QMetaType preferred_type) const override;
	QList<FORMATETC> formatsForMime(const QString& mime_type, const QMimeData* mime_data) const override;
	QString mimeForFormat(const FORMATETC& format) const override;

private:
	FORMATETC initFormat() const;

private:
	int CF_RTF;
};

#endif
