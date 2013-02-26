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

#include "document_watcher.h"

#include "document.h"

#include <QApplication>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTextEdit>

//-----------------------------------------------------------------------------

DocumentWatcher* DocumentWatcher::m_instance = 0;

//-----------------------------------------------------------------------------

DocumentWatcher::DocumentWatcher(QObject* parent) :
	QObject(parent)
{
	m_instance = this;
	m_watcher = new QFileSystemWatcher(this);
	connect(m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(documentChanged(QString)));
}

//-----------------------------------------------------------------------------

DocumentWatcher::~DocumentWatcher()
{
	if (m_instance == this) {
		m_instance = 0;
	}
}

//-----------------------------------------------------------------------------

bool DocumentWatcher::isWatching(const QString& path) const
{
	return m_watcher->files().contains(QFileInfo(path).canonicalFilePath());
}

//-----------------------------------------------------------------------------

void DocumentWatcher::addWatch(Document* document)
{
	QString path = QFileInfo(document->filename()).canonicalFilePath();
	if (isWatching(path)) {
		return;
	}
	m_paths.insert(path, document);
	m_watcher->addPath(path);
}

//-----------------------------------------------------------------------------

void DocumentWatcher::removeWatch(Document* document)
{
	// Find path
	QString path = m_paths.key(document);
	if (path.isEmpty()) {
		return;
	}

	// Remove path
	m_paths.remove(path);
	m_watcher->removePath(path);
}

//-----------------------------------------------------------------------------

void DocumentWatcher::processUpdates()
{
	while (!m_updates.isEmpty()) {
		QString path = m_updates.takeFirst();
		QString filename = "<i>" + QFileInfo(path).fileName() + "</i>";

		// Show document
		Document* document = m_paths.value(path);
		if (!document) {
			continue;
		}
		emit showDocument(document);

		if (QFile::exists(path)) {
			// Process changed file
			QMessageBox mbox(document->window());
			mbox.setIcon(QMessageBox::Warning);
			mbox.setWindowTitle(tr("File Changed"));
			mbox.setText(tr("The file %1 was changed by another program.").arg(filename));
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

			QAbstractButton* save_button = mbox.button(QMessageBox::Save);

			QAbstractButton* ignore_button = mbox.button(QMessageBox::Ignore);
			if (ignore_button->icon().isNull() && ignore_button->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
				ignore_button->setIcon(ignore_button->style()->standardIcon(QStyle::SP_MessageBoxWarning));
			}

			mbox.exec();
			if (mbox.clickedButton() == save_button) {
				document->save();
			} else if (mbox.clickedButton() == ignore_button) {
				document->text()->document()->setModified(true);
			} else {
				emit closeDocument(document);
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
		processUpdates();
	}
}

//-----------------------------------------------------------------------------
