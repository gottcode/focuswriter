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

#ifndef DOCUMENT_WATCHER_H
#define DOCUMENT_WATCHER_H

class Document;

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QList>
#include <QObject>
class QFileInfo;
class QFileSystemWatcher;

class DocumentWatcher : public QObject
{
	Q_OBJECT

	struct Details
	{
		Details() :
			permissions(0),
			ignored(false)
		{ }

		Details(const QFileInfo& info);

		QString path;
		QDateTime modified;
		QFile::Permissions permissions;
		bool ignored;
	};

public:
	DocumentWatcher(QObject* parent = 0);
	~DocumentWatcher();

	bool isWatching(const QString& path) const;

	void addWatch(Document* document);
	void pauseWatch(Document* document);
	void removeWatch(Document* document);
	void resumeWatch(Document* document);
	void updateWatch(Document* document);

	static DocumentWatcher* instance()
	{
		return m_instance;
	}

public slots:
	void processUpdates();

signals:
	void closeDocument(Document* document);
	void showDocument(Document* document);

private slots:
	void documentChanged(const QString& path);

private:
	QFileSystemWatcher* m_watcher;
	QHash<Document*, Details> m_documents;
	QHash<QString, Document*> m_paths;
	QList<QString> m_updates;
	static DocumentWatcher* m_instance;
};

#endif
