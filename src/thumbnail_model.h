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

#ifndef THUMBNAIL_MODEL_H
#define THUMBNAIL_MODEL_H

#include <QDirModel>
class ThumbnailLoader;

class ThumbnailModel : public QDirModel {
	Q_OBJECT
public:
	ThumbnailModel(QObject* parent = 0);
	~ThumbnailModel();

	void clear();

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

private slots:
	void generated(const QString& file);

private:
	ThumbnailLoader* m_loader;
	QPixmap m_loading;
};

#endif
