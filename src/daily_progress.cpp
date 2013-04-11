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

#include "daily_progress.h"

#include "preferences.h"

#include <QDate>
#include <QSettings>

//-----------------------------------------------------------------------------

DailyProgress::DailyProgress(QObject* parent) :
	QObject(parent),
	m_words(0),
	m_msecs(0),
	m_type(0),
	m_goal(0)
{
	QSettings settings;
	if (settings.value("Progress/Date").toDate() != QDate::currentDate()) {
		settings.remove("Progress");
	}
	settings.setValue("Progress/Date", QDate::currentDate().toString(Qt::ISODate));
	m_words = settings.value("Progress/Words", 0).toInt();
	m_msecs = settings.value("Progress/Time", 0).toInt();
}

//-----------------------------------------------------------------------------

DailyProgress::~DailyProgress()
{
	save();
}

//-----------------------------------------------------------------------------

int DailyProgress::percentComplete() const
{
	int progress = 0;
	if (m_type == 1) {
		progress = (m_msecs * 100) / m_goal;
	} else if (m_type == 2) {
		progress = (m_words * 100) / m_goal;
	}
	return progress;
}

//-----------------------------------------------------------------------------

void DailyProgress::loadPreferences(const Preferences& preferences)
{
	m_type = preferences.goalType();
	if (m_type == 1) {
		m_goal = preferences.goalMinutes() * 60000;
	} else if (m_type == 2) {
		m_goal = preferences.goalWords();
	} else {
		m_goal = 0;
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::save()
{
	QSettings settings;
	settings.setValue("Progress/Words", m_words);
	settings.setValue("Progress/Time", m_msecs);
}

//-----------------------------------------------------------------------------
