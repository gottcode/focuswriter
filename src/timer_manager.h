/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_TIMER_MANAGER_H
#define FOCUSWRITER_TIMER_MANAGER_H

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
	explicit TimerManager(Stack* documents, QWidget* parent = nullptr);

	bool cancelEditing();
	TimerDisplay* display() const;

public Q_SLOTS:
	void saveTimers();

protected:
	void closeEvent(QCloseEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void showEvent(QShowEvent* event) override;

private Q_SLOTS:
	void newTimer();
	void recentTimer(const QAction* action);
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

#endif // FOCUSWRITER_TIMER_MANAGER_H
