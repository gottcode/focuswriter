/***********************************************************************
 *
 * Copyright (C) 2013, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "daily_progress_label.h"

#include "daily_progress.h"

//-----------------------------------------------------------------------------

DailyProgressLabel::DailyProgressLabel(DailyProgress* progress, QWidget* parent) :
	QLabel(parent),
	m_progress(progress)
{
	setText(tr("%L1% of daily goal").arg(0));
	connect(m_progress, &DailyProgress::progressChanged, this, &DailyProgressLabel::progressChanged);
}

//-----------------------------------------------------------------------------

void DailyProgressLabel::mousePressEvent(QMouseEvent* event)
{
	emit clicked();

	QLabel::mousePressEvent(event);
}

//-----------------------------------------------------------------------------

void DailyProgressLabel::progressChanged()
{
	setText(tr("%L1% of daily goal").arg(m_progress->percentComplete()));
}

//-----------------------------------------------------------------------------
