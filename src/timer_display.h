/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef TIMER_DISPLAY_H
#define TIMER_DISPLAY_H

class Timer;

#include <QWidget>
class QDateTime;
class QTimer;

class TimerDisplay : public QWidget
{
	Q_OBJECT

public:
	TimerDisplay(QList<Timer*>& timers, QWidget* parent = 0);

	void setTimer(Timer* timer);

	virtual QSize minimumSizeHint() const;
	virtual QSize sizeHint() const;

signals:
	void clicked();

protected:
	virtual void changeEvent(QEvent* event);
	virtual bool event(QEvent* event);
	virtual void hideEvent(QHideEvent* event);
	virtual void leaveEvent(QEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void paintEvent(QPaintEvent* event);
	virtual void showEvent(QShowEvent* event);

private:
	int m_size;
	bool m_show_tip;
	QPoint m_tip_pos;
	QTimer* m_update_timer;

	Timer* m_timer;
	QList<Timer*>& m_timers;
};

#endif
