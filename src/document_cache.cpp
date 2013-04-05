/***********************************************************************
 *
 * Copyright (C) 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "document_cache.h"

#include "document_writer.h"

#include <QDir>
#include <QFile>

//-----------------------------------------------------------------------------

QString DocumentCache::m_path;

//-----------------------------------------------------------------------------

DocumentCache::DocumentCache(QObject* parent) :
	QObject(parent)
{
}

//-----------------------------------------------------------------------------

DocumentCache::~DocumentCache()
{
	// Empty cache
	QDir dir(m_path);
	QStringList files = dir.entryList(QDir::Files);
	foreach (const QString& file, files) {
		dir.remove(file);
	}
}

//-----------------------------------------------------------------------------

QString DocumentCache::path()
{
	return m_path;
}

//-----------------------------------------------------------------------------

void DocumentCache::setPath(const QString& path)
{
	m_path = path;
	if (!m_path.endsWith(QLatin1Char('/'))) {
		m_path += QLatin1Char('/');
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::cacheFile(DocumentWriter* document)
{
	document->write();
	delete document;
}

//-----------------------------------------------------------------------------

void DocumentCache::removeCacheFile(const QString& document)
{
	QFile::remove(document);
}

//-----------------------------------------------------------------------------
