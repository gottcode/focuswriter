/*
	SPDX-FileCopyrightText: 2013-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "daily_progress_label.h"

#include "daily_progress.h"

//-----------------------------------------------------------------------------

DailyProgressLabel::DailyProgressLabel(DailyProgress* progress, QWidget* parent)
	: QLabel(parent)
	, m_progress(progress)
{
	setText(tr("%L1% of daily goal").arg(0));
	connect(m_progress, &DailyProgress::progressChanged, this, &DailyProgressLabel::progressChanged);
}

//-----------------------------------------------------------------------------

void DailyProgressLabel::mousePressEvent(QMouseEvent* event)
{
	Q_EMIT clicked();

	QLabel::mousePressEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressLabel::progressChanged()
{
	setText(tr("%L1% of daily goal").arg(m_progress->percentComplete()));
}

//-----------------------------------------------------------------------------
