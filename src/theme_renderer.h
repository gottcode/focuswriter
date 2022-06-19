/*
	SPDX-FileCopyrightText: 2014-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_THEME_RENDERER_H
#define FOCUSWRITER_THEME_RENDERER_H

#include "theme.h"

#include <QImage>
#include <QMutex>
#include <QRect>
#include <QThread>

class ThemeRenderer : public QThread
{
	Q_OBJECT

public:
	explicit ThemeRenderer(QObject* parent = nullptr);

	void create(const Theme& theme, const QSize& background, const int margin, const qreal pixelratio);

Q_SIGNALS:
	void rendered(const QImage& image, const QRect& foreground, const Theme& theme);

protected:
	void run() override;

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

#endif // FOCUSWRITER_THEME_RENDERER_H
