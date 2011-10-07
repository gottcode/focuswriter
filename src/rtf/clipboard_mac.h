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

#ifndef RTF_CLIPBOARD_MAC_H
#define RTF_CLIPBOARD_MAC_H

#include <QMacPasteboardMime>

namespace RTF
{
	class Clipboard : public QMacPasteboardMime
	{
	public:
		Clipboard()
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

#endif
