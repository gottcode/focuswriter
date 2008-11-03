/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "image_dialog.h"

#include "thumbnail_model.h"

#include <QDialogButtonBox>
#include <QDirModel>
#include <QFileDialog>
#include <QHeaderView>
#include <QListView>
#include <QSettings>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

/*****************************************************************************/

ImageDialog::ImageDialog(QWidget* parent)
: QDialog(parent) {
	setWindowTitle(tr("Open Image"));

	// Setup folders
	m_folders = new QDirModel(this);
	m_folders->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	m_folders->setResolveSymlinks(false);
	m_folders->setSorting(QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

	m_folders_view = new QTreeView;
	m_folders_view->setModel(m_folders);
	m_folders_view->header()->hide();
	m_folders_view->setColumnHidden(1, true);
	m_folders_view->setColumnHidden(2, true);
	m_folders_view->setColumnHidden(3, true);
	m_folders_view->setRootIndex(m_folders->index("/"));
	connect(m_folders_view, SIGNAL(clicked(const QModelIndex&)), this, SLOT(folderClicked(const QModelIndex&)));

	// Setup files
	m_files = new ThumbnailModel(this);

	m_files_view = new QListView;
	m_files_view->setModel(m_files);
	m_files_view->setSpacing(12);
	m_files_view->setViewMode(QListView::IconMode);
	m_files_view->setIconSize(QSize(100, 100));
	m_files_view->setMovement(QListView::Static);
	m_files_view->setResizeMode(QListView::Adjust);
	m_files_view->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_files_view, SIGNAL(activated(const QModelIndex&)), this, SLOT(accept()));

	// Position folder and file views
	m_contents = new QSplitter(this);
	m_contents->addWidget(m_folders_view);
	m_contents->addWidget(m_files_view);
	m_contents->setStretchFactor(1, 1);

	// Setup buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(12);
	layout->setSpacing(24);
	layout->addWidget(m_contents);
	layout->addWidget(buttons);

	// Resize dialog and splitter
	QSettings settings("GottCode");
	resize(settings.value("OpenImage/Size", QSize(675, 430)).toSize());
	m_contents->restoreState(settings.value("OpenImage/Splitter").toByteArray());

	// Default to home directory
	setPath(QDir::homePath());
}

/*****************************************************************************/

QString ImageDialog::selectedFile() const {
	QStringList files = selectedFiles();
	if (!files.isEmpty()) {
		return files.first();
	} else {
		return QString();
	}
}

/*****************************************************************************/

QStringList ImageDialog::selectedFiles() const {
	QStringList result;
	QModelIndexList indexes = m_files_view->selectionModel()->selectedIndexes();
	foreach (const QModelIndex& index, indexes) {
		result.append(m_files->filePath(index));
	}
	return result;
}

/*****************************************************************************/

void ImageDialog::setMultipleSelections(bool multiple) {
	if (multiple) {
		m_files_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
	} else {
		m_files_view->setSelectionMode(QAbstractItemView::SingleSelection);
	}
}

/*****************************************************************************/

void ImageDialog::setPath(const QString& path) {
	QFileInfo info(path);
	if (!info.exists()) {
		return;
	}

	// Find directory
	QString dir = !info.isDir() ? info.absolutePath() : info.absoluteFilePath();

	// Expand directory
	QModelIndex index = m_folders->index(dir);
	m_folders_view->collapseAll();
	m_folders_view->expand(index);
	m_folders_view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
	m_folders_view->scrollTo(index);

	// Select file
	m_files_view->setRootIndex(m_files->index(dir));
	if (info.isFile()) {
		m_files_view->selectionModel()->select(m_files->index(path), QItemSelectionModel::Select);
	}
}

/*****************************************************************************/

void ImageDialog::hideEvent(QHideEvent* event) {
	QSettings settings("GottCode");
	settings.setValue("OpenImage/Size", size());
	settings.setValue("OpenImage/Splitter", m_contents->saveState());
	QDialog::hideEvent(event);
}

/*****************************************************************************/

void ImageDialog::folderClicked(const QModelIndex& index) {
	m_files->clear();
	m_files_view->setRootIndex(m_files->index(m_folders->filePath(index)));
}

/*****************************************************************************/
