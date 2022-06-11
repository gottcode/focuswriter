/*
	SPDX-FileCopyrightText: 2012-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DOCUMENT_CACHE_H
#define FOCUSWRITER_DOCUMENT_CACHE_H

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
	explicit DocumentCache(QObject* parent = nullptr);
	~DocumentCache();

	bool isClean() const;
	bool isWritable() const;
	void parseMapping(QStringList& files, QStringList& datafiles) const;

	void add(const Document* document);
	void remove(const Document* document);
	void setOrdering(Stack* ordering);

	static void setPath(const QString& path);

public Q_SLOTS:
	void updateMapping() const;

private Q_SLOTS:
	void replaceCacheFile(const Document* document, const QString& file);
	void writeCacheFile(const Document* document, QSharedPointer<DocumentWriter> writer);

private:
	QString backupCache() const;
	QString createFileName() const;
	void updateCacheFile(const Document* document, const QString& cache_file);

private:
	Stack* m_ordering;
	QHash<const Document*, QString> m_filenames;
	QString m_previous_cache;

	static QString m_path;
};

#endif // FOCUSWRITER_DOCUMENT_CACHE_H
