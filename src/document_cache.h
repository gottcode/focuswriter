/*
	SPDX-FileCopyrightText: 2012-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DOCUMENT_CACHE_H
#define DOCUMENT_CACHE_H

#include "document_writer.h"
class Document;
class Stack;

#include <QHash>
#include <QObject>
#include <QSharedPointer>

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
	void writeCacheFile(Document* document, QSharedPointer<DocumentWriter> writer);

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
