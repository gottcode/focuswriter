/***********************************************************************
 *
 * Copyright (C) 2014, 2015, 2016 Graeme Gott <graeme@gottcode.org>
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

#ifndef THEME_RENDERER_H
#define THEME_RENDERER_H

#include "theme.h"

#include <QImage>
#include <QMutex>
#include <QRect>
#include <QThread>

class ThemeRenderer : public QThread
{
	Q_OBJECT

public:
	ThemeRenderer(QObject* parent = 0);

	void create(const Theme& theme, const QSize& background, const int margin, const qreal pixelratio);

signals:
	void rendered(const QImage& image, const QRect& foreground, const Theme& theme);

protected:
	virtual void run();

private:
	struct CacheFile
	{
		Theme theme;
		QSize background;
		QRect foreground;
		QImage image;
		int margin;
		qreal pixelratio;

		bool operator==(const CacheFile& other) const
		{
			return (theme == other.theme) &&
					(background == other.background) &&
					(margin == other.margin) &&
					qFuzzyCompare(pixelratio, other.pixelratio);
		}
	};
	QList<CacheFile> m_files;
	QMutex m_file_mutex;

	QList<CacheFile> m_cache;
};

#endif
