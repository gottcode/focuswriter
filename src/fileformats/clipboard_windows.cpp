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

#include "clipboard_windows.h"

#include <QMimeData>

//-----------------------------------------------------------------------------

RTF::Clipboard::Clipboard() :
	QWinMime()
{
	CF_RTF = QWinMime::registerMimeType(QLatin1String("Rich Text Format"));
}

//-----------------------------------------------------------------------------

bool RTF::Clipboard::canConvertFromMime(const FORMATETC& format, const QMimeData* mime_data) const
{
	return (format.cfFormat == CF_RTF) && mime_data->hasFormat(QLatin1String("text/rtf"));
}

//-----------------------------------------------------------------------------

bool RTF::Clipboard::canConvertToMime(const QString& mime_type, IDataObject* data_obj) const
{
	bool result = false;
	if (mime_type == QLatin1String("text/rtf")) {
		FORMATETC format = initFormat();
		format.tymed |= TYMED_ISTREAM;
		result = (data_obj->QueryGetData(&format) == S_OK);
	}
	return result;
}

//-----------------------------------------------------------------------------

bool RTF::Clipboard::convertFromMime(const FORMATETC& format, const QMimeData* mime_data, STGMEDIUM* storage_medium) const
{
	if (canConvertFromMime(format, mime_data)) {
		QByteArray data = mime_data->data(QLatin1String("text/rtf"));

		HANDLE data_handle = GlobalAlloc(0, data.size());
		if (!data_handle) {
			return false;
		}
		void* data_ptr = GlobalLock(data_handle);
		memcpy(data_ptr, data.data(), data.size());
		GlobalUnlock(data_handle);

		storage_medium->tymed = TYMED_HGLOBAL;
		storage_medium->hGlobal = data_handle;
		storage_medium->pUnkForRelease = NULL;

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

QVariant RTF::Clipboard::convertToMime(const QString& mime_type, IDataObject* data_obj, QVariant::Type preferred_type) const
{
	Q_UNUSED(preferred_type);

	QVariant result;
	if (canConvertToMime(mime_type, data_obj)) {
		QByteArray data;
		FORMATETC format = initFormat();
		format.tymed |= TYMED_ISTREAM;
		STGMEDIUM storage_medium;

		if (data_obj->GetData(&format, &storage_medium) == S_OK) {
			if (storage_medium.tymed == TYMED_HGLOBAL) {
				char* data_ptr = reinterpret_cast<char*>(GlobalLock(storage_medium.hGlobal));
				data = QByteArray::fromRawData(data_ptr, GlobalSize(storage_medium.hGlobal));
				data.detach();
				GlobalUnlock(storage_medium.hGlobal);
			} else if (storage_medium.tymed == TYMED_ISTREAM) {
				char buffer[4096];
				ULONG amount_read = 0;
				LARGE_INTEGER pos = {{0, 0}};
				HRESULT stream_result = storage_medium.pstm->Seek(pos, STREAM_SEEK_SET, NULL);
				while (SUCCEEDED(stream_result)) {
					stream_result = storage_medium.pstm->Read(buffer, sizeof(buffer), &amount_read);
					if (SUCCEEDED(stream_result) && (amount_read > 0)) {
						data += QByteArray::fromRawData(buffer, amount_read);
					}
					if (amount_read != sizeof(buffer)) {
						break;
					}
				}
				data.detach();
			}
			ReleaseStgMedium(&storage_medium);
		}

		if (!data.isEmpty()) {
			result = data;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

QVector<FORMATETC> RTF::Clipboard::formatsForMime(const QString& mime_type, const QMimeData* mime_data) const
{
	QVector<FORMATETC> result;
	if ((mime_type == QLatin1String("text/rtf")) && mime_data->hasFormat(QLatin1String("text/rtf"))) {
		result += initFormat();
	}
	return result;
}

//-----------------------------------------------------------------------------

QString RTF::Clipboard::mimeForFormat(const FORMATETC& format) const
{
	if (format.cfFormat == CF_RTF) {
		return QLatin1String("text/rtf");
	}
	return QString();
}

//-----------------------------------------------------------------------------

FORMATETC RTF::Clipboard::initFormat() const
{
	FORMATETC format;
	format.cfFormat = CF_RTF;
	format.ptd = NULL;
	format.dwAspect = DVASPECT_CONTENT;
	format.lindex = -1;
	format.tymed = TYMED_HGLOBAL;
	return format;
}

//-----------------------------------------------------------------------------
