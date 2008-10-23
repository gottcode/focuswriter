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

#include "thumbnail_loader.h"

#include <QColor>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QSize>

/*****************************************************************************/

ThumbnailLoader::ThumbnailLoader()
: m_done(false) {
}

/*****************************************************************************/

void ThumbnailLoader::add(const QString& file, const QString& preview) {
	m_thumbnails_mutex.lock();
	Thumbnail thumb = { file, preview };
	m_thumbnails.prepend(thumb);
	m_thumbnails_mutex.unlock();

	if (!isRunning()) {
		start();
	}
}

/*****************************************************************************/

void ThumbnailLoader::stop() {
	m_done_mutex.lock();
	m_done = true;
	m_done_mutex.unlock();
}

/*****************************************************************************/

void ThumbnailLoader::clear() {
	m_thumbnails_mutex.lock();
	m_thumbnails.clear();
	m_thumbnails_mutex.unlock();
}

/*****************************************************************************/

void ThumbnailLoader::run() {
	Thumbnail details;
	forever {
		// Fetch next thumbnail to process
		m_thumbnails_mutex.lock();
		if (m_thumbnails.isEmpty()) {
			m_thumbnails_mutex.unlock();
			break;
		}
		Thumbnail details = m_thumbnails.takeFirst();
		m_thumbnails_mutex.unlock();

		// Skip already generated previews
		if (QFileInfo(details.preview).exists()) {
			continue;
		}

		// Check for stop
		m_done_mutex.lock();
		if (m_done) {
			m_done_mutex.unlock();
			break;
		}
		m_done_mutex.unlock();

		// Generate thumbnail
		QImageReader source(details.file);
		QSize size = source.size();
		if (size.width() > 92 || size.height() > 92) {
			size.scale(92, 92, Qt::KeepAspectRatio);
			source.setScaledSize(size);
		}

		QImage thumbnail(100, 100, QImage::Format_ARGB32);
		thumbnail.fill(0);
		{
			QPainter painter(&thumbnail);
			painter.translate(46 - (size.width() / 2), 46 - (size.height() / 2));
			painter.fillRect(0, 0, size.width() + 8, size.height() + 8, QColor(0, 0, 0, 50));
			painter.fillRect(1, 1, size.width() + 6, size.height() + 6, QColor(0, 0, 0, 75));
			painter.fillRect(2, 2, size.width() + 4, size.height() + 4, Qt::white);
			painter.drawImage(4, 4, source.read(), 0, 0, -1, -1, Qt::AutoColor | Qt::AvoidDither);
		}
		thumbnail.save(details.preview, 0, 0);

		emit generated(details.file);
	}
}

/*****************************************************************************/
