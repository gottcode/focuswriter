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

#ifndef DOCUMENT_WATCHER_H
#define DOCUMENT_WATCHER_H

#include <QHash>
#include <QSet>
class Document;

#include <QObject>
class QFileSystemWatcher;

class DocumentWatcher : public QObject
{
	Q_OBJECT

public:
	DocumentWatcher(QObject* parent = 0);
	~DocumentWatcher();

	void addWatch(Document* document);
	void removeWatch(Document* document);
	void processUpdates();

	static DocumentWatcher* instance()
	{
		return m_instance;
	}

signals:
	void closeDocument(Document* document);
	void showDocument(Document* document);

private slots:
	void documentChanged(const QString& path);

private:
	QFileSystemWatcher* m_watcher;
	QHash<QString, Document*> m_paths;
	QList<QString> m_updates;
	static DocumentWatcher* m_instance;
};

#endif
