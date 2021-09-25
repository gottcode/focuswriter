/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DAILY_PROGRESS_LABEL_H
#define FOCUSWRITER_DAILY_PROGRESS_LABEL_H

class DailyProgress;

#include <QLabel>

class DailyProgressLabel : public QLabel
{
	Q_OBJECT

public:
	DailyProgressLabel(DailyProgress* progress, QWidget* parent = 0);

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent* event);

private slots:
	void progressChanged();

private:
	DailyProgress* m_progress;
};

#endif // FOCUSWRITER_DAILY_PROGRESS_LABEL_H
