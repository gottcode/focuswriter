/*
	SPDX-FileCopyrightText: 2012-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DOCUMENT_WATCHER_H
#define FOCUSWRITER_DOCUMENT_WATCHER_H

class Document;

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QObject>
#include <QStringList>
class QFileInfo;
class QFileSystemWatcher;

class DocumentWatcher : public QObject
{
	Q_OBJECT

	struct Details
	{
		Details()
			: ignored(false)
		{
		}

		Details(const QFileInfo& info);

		QString path;
		QDateTime modified;
		QFile::Permissions permissions;
		bool ignored;
	};

public:
	explicit DocumentWatcher(QObject* parent = nullptr);
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

public Q_SLOTS:
	void processUpdates();

Q_SIGNALS:
	void closeDocument(Document* document);
	void showDocument(Document* document);

private Q_SLOTS:
	void documentChanged(const QString& path);

private:
	QFileSystemWatcher* m_watcher;
	QHash<const Document*, Details> m_documents;
	QHash<QString, Document*> m_paths;
	QStringList m_updates;
	static DocumentWatcher* m_instance;
};

#endif // FOCUSWRITER_DOCUMENT_WATCHER_H
