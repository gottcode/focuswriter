/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef DAILY_PROGRESS_DIALOG_H
#define DAILY_PROGRESS_DIALOG_H

class DailyProgress;

#include <QDialog>
class QDate;
class QLabel;
class QTableView;

class DailyProgressDialog : public QDialog
{
	Q_OBJECT

public:
	DailyProgressDialog(DailyProgress* progress, QWidget* parent = 0);

	void loadPreferences();

signals:
	void visibleChanged(bool visible);

protected:
	void changeEvent(QEvent* event);
	void closeEvent(QCloseEvent* event);
	void hideEvent(QHideEvent* event);
	void showEvent(QShowEvent* event);

private slots:
	void modelReset();
	void streaksChanged();

private:
	QString createStreakText(const QString& title, const QDate& start, const QDate& end);

private:
	DailyProgress* m_progress;
	QTableView* m_display;
	class Delegate;
	Delegate* m_delegate;
	QWidget* m_streaks;
	QLabel* m_longest_streak;
	QLabel* m_current_streak;
};

#endif
