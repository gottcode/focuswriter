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

#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

class Stack;
class Timer;
class TimerDisplay;

#include <QDialog>
class QLabel;
class QMenu;
class QScrollArea;
class QTimer;
class QVBoxLayout;

class TimerManager : public QDialog
{
	Q_OBJECT

public:
	TimerManager(Stack* documents, QWidget* parent = 0);

	bool cancelEditing();
	TimerDisplay* display() const;

public slots:
	void saveTimers();

protected:
	virtual void closeEvent(QCloseEvent* event);
	virtual void hideEvent(QHideEvent* event);
	virtual void showEvent(QShowEvent* event);

private slots:
	void newTimer();
	void recentTimer(QAction* action);
	void recentTimerMenuRequested(const QPoint& pos);
	void timerChanged(Timer* timer);
	void timerDeleted(QObject* object);
	void timerEdited(Timer* timer);
	void toggleVisibility();
	void updateClock();

private:
	void addTimer(Timer* timer);
	void setupRecentMenu();
	void startClock();
	void updateDisplay();

private:
	Stack* m_documents;
	TimerDisplay* m_display;

	QLabel* m_clock_label;
	QTimer* m_clock_timer;

	QList<Timer*> m_timers;
	QVBoxLayout* m_timers_layout;
	QScrollArea* m_timers_area;

	QPushButton* m_new_button;
	QPushButton* m_recent_button;
	QMenu* m_recent_timers;
};

#endif
