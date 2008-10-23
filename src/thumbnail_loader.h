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

#ifndef THUMBNAIL_LOADER_H
#define THUMBNAIL_LOADER_H

#include <QMutex>
#include <QThread>

class ThumbnailLoader : public QThread {
	Q_OBJECT
public:
	ThumbnailLoader();

	void add(const QString& file, const QString& preview);
	void stop();

public slots:
	void clear();

signals:
	void generated(const QString& file);

protected:
	virtual void run();

private:
	bool m_done;
	struct Thumbnail {
		QString file;
		QString preview;
	};
	QList<Thumbnail> m_thumbnails;
	QMutex m_done_mutex;
	QMutex m_thumbnails_mutex;
};

#endif
