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
	explicit TimerDisplay(const QList<Timer*>& timers, QWidget* parent = nullptr);

	void setTimer(const Timer* timer);

	virtual QSize minimumSizeHint() const override;
	virtual QSize sizeHint() const override;

Q_SIGNALS:
	void clicked();

protected:
	void changeEvent(QEvent* event) override;
	bool event(QEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void showEvent(QShowEvent* event) override;

private:
	int m_size;
	bool m_show_tip;
	QPoint m_tip_pos;
	QTimer* m_update_timer;

	const Timer* m_timer;
	const QList<Timer*>& m_timers;
};

#endif // FOCUSWRITER_TIMER_DISPLAY_H
