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

#include "document.h"
#include "document_writer.h"
#include "stack.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

//-----------------------------------------------------------------------------

QString DocumentCache::m_path;

//-----------------------------------------------------------------------------

DocumentCache::DocumentCache(QObject* parent) :
	QObject(parent),
	m_ordering(0)
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

void DocumentCache::parseMapping(const QString& cache_path, QStringList& files, QStringList& datafiles) const
{
	QFile file(cache_path + "/mapping");
	if (file.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream.setAutoDetectUnicode(true);

		while (!stream.atEnd()) {
			QString line = stream.readLine();
			QString datafile = line.section(' ', 0, 0);
			QString path = line.section(' ', 1);
			if (!datafile.isEmpty()) {
				files.append(path);
				datafiles.append(cache_path + "/" + datafile);
			}
		}
		file.close();
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::add(Document* document)
{
	m_filenames[document] = document->cacheFilename();
	connect(document, SIGNAL(changedName()), this, SLOT(updateMapping()));
	updateMapping();
}

//-----------------------------------------------------------------------------

void DocumentCache::remove(Document* document)
{
	if (m_filenames.contains(document)) {
		QString cache_file = m_filenames.take(document);
		updateMapping();
		QFile::remove(cache_file);
	}
}

//-----------------------------------------------------------------------------

void DocumentCache::setOrdering(Stack* ordering)
{
	m_ordering = ordering;
}

//-----------------------------------------------------------------------------

QString DocumentCache::fileName()
{
	static time_t seed = 0;
	if (seed == 0) {
		seed = time(0);
		qsrand(seed);
	}

	QString filename;
	QDir dir(m_path);
	do {
		filename = QString("fw_%1").arg(qrand(), 6, 36, QLatin1Char('0'));
	} while (dir.exists(filename));

	return filename;
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

void DocumentCache::updateMapping()
{
	QFile file(m_path + "/mapping");
	if (file.open(QFile::WriteOnly | QFile::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream.setGenerateByteOrderMark(true);

		for (int i = 0; i < m_ordering->count(); ++i) {
			Document* document = m_ordering->document(i);
			if (!m_filenames.contains(document)) {
				continue;
			}
			stream << QFileInfo(m_filenames[document]).baseName() << " " << document->filename() << endl;
		}
		file.close();
	}
}

//-----------------------------------------------------------------------------
