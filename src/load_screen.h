/*
	SPDX-FileCopyrightText: 2010-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_LOAD_SCREEN_H
#define FOCUSWRITER_LOAD_SCREEN_H

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

#endif // FOCUSWRITER_LOAD_SCREEN_H
