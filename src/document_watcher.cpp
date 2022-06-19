/*
	SPDX-FileCopyrightText: 2012-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "document_watcher.h"

#include "document.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>

//-----------------------------------------------------------------------------

DocumentWatcher::Details::Details(const QFileInfo& info)
	: path(info.canonicalFilePath())
	, modified(info.lastModified())
	, permissions(info.permissions())
	, ignored(false)
{
}

//-----------------------------------------------------------------------------

DocumentWatcher* DocumentWatcher::m_instance = nullptr;

//-----------------------------------------------------------------------------

DocumentWatcher::DocumentWatcher(QObject* parent)
	: QObject(parent)
{
	m_instance = this;
	m_watcher = new QFileSystemWatcher(this);
	connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &DocumentWatcher::documentChanged);
}

//-----------------------------------------------------------------------------

DocumentWatcher::~DocumentWatcher()
{
	if (m_instance == this) {
		m_instance = nullptr;
	}
}

//-----------------------------------------------------------------------------

bool DocumentWatcher::isWatching(const QString& path) const
{
	return m_paths.contains(QFileInfo(path).canonicalFilePath());
}

//-----------------------------------------------------------------------------

void DocumentWatcher::addWatch(Document* document)
{
	if (m_documents.contains(document)) {
		return;
	}

	// Store document details
	const QString path = document->filename();
	if (!path.isEmpty()) {
		m_documents.insert(document, QFileInfo(path));
		const Details& details = m_documents[document];

		// Add path
		m_paths.insert(details.path, document);
		m_watcher->addPath(details.path);
	} else {
		m_documents.insert(document, Details());
	}
}

//-----------------------------------------------------------------------------

void DocumentWatcher::pauseWatch(Document* document)
{
	// Ignore path
	Details& details = m_documents[document];
	details.ignored = true;

	// Remove watch
	if (!details.path.isEmpty()) {
		m_watcher->removePath(details.path);
		details.path.clear();
	}
}

//-----------------------------------------------------------------------------

void DocumentWatcher::removeWatch(Document* document)
{
	// Remove document details
	const Details details = m_documents.take(document);

	// Remove path
	if (!details.path.isEmpty()) {
		m_watcher->removePath(details.path);
		m_paths.remove(details.path);
	}
}

//-----------------------------------------------------------------------------

void DocumentWatcher::resumeWatch(Document* document)
{
	m_documents[document].ignored = false;
	updateWatch(document);
}

//-----------------------------------------------------------------------------

void DocumentWatcher::updateWatch(Document* document)
{
	// Update document details
	Details& details = m_documents[document];
	const QString oldpath = details.path;
	const QString path = document->filename();
	if (!path.isEmpty()) {
		const QFileInfo info(path);
		details.path = info.canonicalFilePath();
		details.modified = info.lastModified();
		details.permissions = info.permissions();
	} else {
		details.path = path;
		details.modified = QDateTime();
		details.permissions = QFile::Permissions();
	}

	// Update path
	if (details.path != oldpath) {
		// Remove old path
		if (!oldpath.isEmpty()) {
			m_watcher->removePath(oldpath);
			m_paths.remove(oldpath);
		}

		// Add new path
		if (!path.isEmpty()) {
			m_paths.insert(details.path, document);
			m_watcher->addPath(details.path);
		}
	}
}

//-----------------------------------------------------------------------------

void DocumentWatcher::processUpdates()
{
	while (!m_updates.isEmpty()) {
		const QString path = m_updates.takeFirst();
		const QFileInfo info(path);
		const QString filename = info.fileName();

		// Find document
		Document* document = m_paths.value(path);
		if (!document) {
			continue;
		}
		const Details& details = m_documents[document];
		if (details.ignored) {
			continue;
		}

		// Ignore unchanged documents
		if (info.exists() && (details.modified == info.lastModified()) && (details.permissions == info.permissions())) {
			continue;
		}

		// Show document
		Q_EMIT showDocument(document);

		if (info.exists()) {
			// Process changed file
			QMessageBox mbox(document->window());
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(tr("File Changed"));
			mbox.setText(tr("The file '%1' was changed by another program.").arg(filename));
			mbox.setInformativeText(tr("Do you want to reload the file?"));

			QPushButton* reload_button = mbox.addButton(tr("Reload"), QMessageBox::AcceptRole);
			if (reload_button->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
				reload_button->setIcon(reload_button->style()->standardIcon(QStyle::SP_BrowserReload));
			}
			QPushButton* ignore_button = mbox.addButton(QMessageBox::Cancel);
			ignore_button->setText(tr("Ignore"));
			mbox.setDefaultButton(reload_button);

			mbox.exec();
			if (mbox.clickedButton() == reload_button) {
				document->reload(false);
			}
		} else  {
			// Process deleted file
			QMessageBox mbox(document->window());
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(tr("File Deleted"));
			mbox.setText(tr("The file %1 was deleted by another program.").arg(filename));
			mbox.setInformativeText(tr("Do you want to save or close the file?"));

			mbox.setStandardButtons(QMessageBox::Save | QMessageBox::Close | QMessageBox::Ignore);
			mbox.setDefaultButton(QMessageBox::Save);

			const QAbstractButton* save_button = mbox.button(QMessageBox::Save);

			QAbstractButton* ignore_button = mbox.button(QMessageBox::Ignore);
			if (ignore_button->icon().isNull() && ignore_button->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
				ignore_button->setIcon(ignore_button->style()->standardIcon(QStyle::SP_MessageBoxWarning));
			}

			mbox.exec();
			if (mbox.clickedButton() == save_button) {
				document->save();
			} else if (mbox.clickedButton() == ignore_button) {
				document->setModified(true);
			} else {
				Q_EMIT closeDocument(document);
			}
		}
	}
}

//-----------------------------------------------------------------------------

void DocumentWatcher::documentChanged(const QString& path)
{
	if (m_updates.contains(path)) {
		return;
	}
	m_updates.append(path);
	if (parent() && (QApplication::activeWindow() == parent())) {
		QTimer::singleShot(200, this, &DocumentWatcher::processUpdates);
	}
}

//-----------------------------------------------------------------------------
