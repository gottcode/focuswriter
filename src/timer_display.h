/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_TIMER_DISPLAY_H
#define FOCUSWRITER_TIMER_DISPLAY_H

class Timer;

#include <QWidget>
class QDateTime;
class QTimer;

class TimerDisplay : public QWidget
{
	Q_OBJECT

public:
	explicit TimerDisplay(QList<Timer*>& timers, QWidget* parent = 0);

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

#endif // FOCUSWRITER_TIMER_DISPLAY_H
