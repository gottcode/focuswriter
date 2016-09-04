/***********************************************************************
 *
 * Copyright (C) 2011, 2014 Graeme Gott <graeme@gottcode.org>
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
