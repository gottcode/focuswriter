/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DAILY_PROGRESS_DIALOG_H
#define FOCUSWRITER_DAILY_PROGRESS_DIALOG_H

class DailyProgress;

#include <QDialog>
class QDate;
class QLabel;
class QTableView;

class DailyProgressDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DailyProgressDialog(DailyProgress* progress, QWidget* parent = nullptr);

	void loadPreferences();

Q_SIGNALS:
	void visibleChanged(bool visible);

protected:
	void changeEvent(QEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	void showEvent(QShowEvent* event) override;

private Q_SLOTS:
	void modelReset();
	void streaksChanged();

private:
	QString createStreakText(const QString& title, const QDate& start, const QDate& end) const;

private:
	DailyProgress* m_progress;
	QTableView* m_display;
	class Delegate;
	Delegate* m_delegate;
	QWidget* m_streaks;
	QLabel* m_longest_streak;
	QLabel* m_current_streak;
};

#endif // FOCUSWRITER_DAILY_PROGRESS_DIALOG_H
