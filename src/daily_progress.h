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

#ifndef DAILY_PROGRESS_H
#define DAILY_PROGRESS_H

class Preferences;

#include <QObject>

class DailyProgress : public QObject
{
	Q_OBJECT

public:
	DailyProgress(QObject* parent = 0);
	~DailyProgress();

	int percentComplete() const;

	void increaseWordCount(int words);
	void increaseTime(int msecs);
	void loadPreferences(const Preferences& preferences);
	void save();

private:
	int m_words;
	int m_msecs;
	int m_type;
	int m_goal;
};

inline void DailyProgress::increaseWordCount(int words)
{
	m_words += words;
}

inline void DailyProgress::increaseTime(int msecs)
{
	m_msecs += msecs;
}

#endif
