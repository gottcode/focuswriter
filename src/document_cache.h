/***********************************************************************
 *
 * Copyright (C) 2012 Graeme Gott <graeme@gottcode.org>
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

#ifndef DOCUMENT_CACHE_H
#define DOCUMENT_CACHE_H

class DocumentWriter;

#include <QObject>

class DocumentCache : public QObject
{
	Q_OBJECT

public:
	DocumentCache(QObject* parent = 0);
	~DocumentCache();

public slots:
	void cacheFile(DocumentWriter* document);
	void removeCacheFile(const QString& document);
};

#endif
