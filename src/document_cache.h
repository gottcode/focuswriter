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

#ifndef DOCUMENT_CACHE_H
#define DOCUMENT_CACHE_H

class Document;
class DocumentWriter;
class Stack;

#include <QHash>
#include <QObject>

class DocumentCache : public QObject
{
	Q_OBJECT

public:
	DocumentCache(QObject* parent = 0);
	~DocumentCache();

	bool isClean() const;
	bool isWritable() const;
	void parseMapping(QStringList& files, QStringList& datafiles) const;

	void add(Document* document);
	void remove(Document* document);
	void setOrdering(Stack* ordering);

	static void setPath(const QString& path);

public slots:
	void updateMapping();

private slots:
	void replaceCacheFile(Document* document, const QString& file);
	void writeCacheFile(Document* document, DocumentWriter* writer);

private:
	QString backupCache();
	QString createFileName();
	void updateCacheFile(Document* document, const QString& cache_file);

private:
	Stack* m_ordering;
	QHash<Document*, QString> m_filenames;
	QString m_previous_cache;

	static QString m_path;
};

#endif
