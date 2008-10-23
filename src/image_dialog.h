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

#ifndef IMAGE_DIALOG_H
#define IMAGE_DIALOG_H

#include <QDialog>
class QDirModel;
class QListView;
class QModelIndex;
class QSplitter;
class QTreeView;
class ThumbnailModel;

class ImageDialog : public QDialog {
	Q_OBJECT
public:
	ImageDialog(QWidget* parent = 0);

	QStringList selectedFiles() const;
	void setDirectory(const QString& directory);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void folderClicked(const QModelIndex& index);

private:
	QSplitter* m_contents;
	QDirModel* m_folders;
	QTreeView* m_folders_view;
	ThumbnailModel* m_files;
	QListView* m_files_view;
};

#endif
