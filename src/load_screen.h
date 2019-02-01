/***********************************************************************
 *
 * Copyright (C) 2010, 2012, 2016 Graeme Gott <graeme@gottcode.org>
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

#ifndef LOAD_SCREEN_H
#define LOAD_SCREEN_H

#include <QLabel>
#include <QPixmap>
class QGraphicsOpacityEffect;
class QTimer;

class LoadScreen : public QLabel
{
	Q_OBJECT

public:
	LoadScreen(QWidget* parent);

	bool eventFilter(QObject* watched, QEvent* event);

public slots:
	void setText(const QString& step);
	void finish();

protected:
	void hideEvent(QHideEvent* event);
	void showEvent(QShowEvent* event);
	void paintEvent(QPaintEvent* event);

private slots:
	void fade();

private:
	QPixmap m_pixmap;
	QSizeF m_pixmap_center;
	QLabel* m_text;
	QGraphicsOpacityEffect* m_hide_effect;
	QTimer* m_hide_timer;
};

#endif
