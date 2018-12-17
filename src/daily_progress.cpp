/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2015, 2016, 2017 Graeme Gott <graeme@gottcode.org>
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

#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QTimer>

#include <cmath>

//-----------------------------------------------------------------------------

QString DailyProgress::m_path;

//-----------------------------------------------------------------------------

DailyProgress::DailyProgress(QObject* parent) :
	QAbstractTableModel(parent),
	m_words(0),
	m_msecs(0),
	m_type(0),
	m_goal(0),
	m_current_valid(false),
	m_current_pos(0),
	m_progress_enabled(0),
	m_streak_minimum(100)
{
	// Fetch date of when the program was started
	QDate date = QDate::currentDate();

	// Initialize daily progress data
	m_file = new QSettings(m_path, QSettings::IniFormat, this);

	int version = m_file->value(QLatin1String("Version"), -1).toInt();
	if (version == 1) {
		// Load current daily progress data from 1.5
		m_file->beginGroup(QLatin1String("Progress"));
		QVariantList values = m_file->value(date.toString(Qt::ISODate)).toList();
		m_words = values.value(0).toInt();
		m_msecs = values.value(1).toInt();
		m_type = values.value(2).toInt();
		m_goal = values.value(3).toInt();

		// Load all daily progress from 1.5
		QStringList keys = m_file->childKeys();
		for (const QString& key : keys) {
			QDate date = QDate::fromString(key, Qt::ISODate);
			if (!date.isValid()) {
				continue;
			}
			QVariantList values = m_file->value(key).toList();
			if (values.size() == 4) {
				m_progress.append(Progress(date, values.at(0).toInt(), values.at(1).toInt(), values.at(2).toInt(), values.at(3).toInt()));
			} else {
				m_progress.append(Progress(date));
			}
		}
	} else if (version == -1) {
		// Load current daily progress data from 1.4
		QSettings settings;
		if (settings.value(QLatin1String("Progress/Date")).toString() == date.toString(Qt::ISODate)) {
			m_words = settings.value(QLatin1String("Progress/Words"), 0).toInt();
			m_msecs = settings.value(QLatin1String("Progress/Time"), 0).toInt();
		}
		settings.remove(QLatin1String("Progress"));
		m_file->setValue(QLatin1String("Version"), 1);
		m_file->beginGroup(QLatin1String("Progress"));
	} else {
		int extra = 0;
		QString newpath = m_path + ".bak";
		while (QFile::exists(newpath)) {
			++extra;
			newpath = m_path + ".bak" + QString::number(extra);
		}
		QFile::copy(m_path, newpath);
		qWarning("The daily progress history is of unsupported version %d and could not be loaded.", version);
	}

	// Make sure there is a current daily progress
	if (m_progress.isEmpty() || (m_progress.last().date() != date)) {
		m_progress.append(Progress(date));
	}

	// Add null entries before data to make it week-based
	QLocale locale;
	int start_of_week = locale.firstDayOfWeek();
	int day_of_week = m_progress.first().date().dayOfWeek();
	int null_days = 0;
	if (day_of_week < start_of_week) {
		null_days = day_of_week + (7 - start_of_week);
	} else if (day_of_week > start_of_week) {
		null_days = day_of_week - start_of_week;
	}
	for (int i = 0; i < null_days; ++i) {
		m_progress.insert(0, Progress());
	}

	// Fetch day names
	day_of_week = start_of_week;
	m_day_names.append(QString());
	for (int i = 0; i < 7; ++i) {
		m_day_names.append(locale.dayName(day_of_week, QLocale::ShortFormat));
		if (day_of_week != Qt::Sunday) {
			++day_of_week;
		} else {
			day_of_week = Qt::Monday;
		}
	}
	m_day_names.append(QString());

	updateRows();

	m_typing_timer.start();

	QTimer* day_timer = new QTimer(this);
	connect(day_timer, SIGNAL(timeout()), this, SLOT(updateDay()));
	day_timer->start(86400000);
}

//-----------------------------------------------------------------------------

DailyProgress::~DailyProgress()
{
	save();
}

//-----------------------------------------------------------------------------

void DailyProgress::findCurrentStreak(QDate& start, QDate& end) const
{
	int start_pos = -1, end_pos = -1;
	findStreak(m_current_pos, start_pos, end_pos);

	if ((start_pos == -1) && (m_current_pos > 0)) {
		findStreak(m_current_pos - 1, start_pos, end_pos);
	}

	if (start_pos != -1) {
		start = m_progress.at(start_pos).date();
		end = m_progress.at(end_pos).date();
	} else {
		start = end = QDate();
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::findLongestStreak(QDate& start, QDate& end) const
{
	int start_pos = -1, end_pos = -1, length = -1;
	for (int i = m_current_pos; i >= 0; --i) {
		int test_start_pos = -1, test_end_pos = -1;
		findStreak(i, test_start_pos, test_end_pos);
		if (test_start_pos != -1) {
			// Track if it is longest streak found so far
			int test_length = test_end_pos - test_start_pos;
			if (test_length > length) {
				start_pos = test_start_pos;
				end_pos = test_end_pos;
				length = test_length;
			}

			// Skip finding streaks inside of a streak
			if (test_length > 0) {
				i -= (test_length - 1);
			}
		}
	}

	if (start_pos != -1) {
		start = m_progress.at(start_pos).date();
		end = m_progress.at(end_pos).date();
	} else {
		start = end = QDate();
	}
}

//-----------------------------------------------------------------------------

int DailyProgress::percentComplete()
{
	if (!m_current_valid) {
		m_current_valid = true;

		bool had_streak_before = m_current->progress() >= m_streak_minimum;
		m_current->setProgress(m_words, m_msecs, m_type, m_goal);
		bool had_streak_after = m_current->progress() >= m_streak_minimum;

		if (had_streak_before != had_streak_after) {
			emit streaksChanged();
		}

		QModelIndex index = createIndex(m_current_pos / 7, m_current_pos % 7);
		emit dataChanged(index, index);

		emit progressChanged();
	}
	return m_current->progress();
}

//-----------------------------------------------------------------------------

void DailyProgress::increaseTime()
{
	const qint64 msecs = m_typing_timer.restart();

	if (msecs < 30000) {
		m_msecs += msecs;
		m_current_valid = false;
		updateProgress();
	} else if (msecs >= 7200000) {
		updateDay();
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::loadPreferences()
{
	// Check if history is disabled
	m_file->endGroup();
	if (Preferences::instance().goalHistory()) {
		m_file->remove(QLatin1String("HistoryDisabled"));
	} else {
		// Remove history of previous launch
		QDate disabled_date = m_file->value(QLatin1String("HistoryDisabled")).toDate();
		if (disabled_date.isValid() && (disabled_date != m_current->date())) {
			m_file->remove(QLatin1String("Progress/") + disabled_date.toString(Qt::ISODate));

			// Update model
			int pos = m_current_pos - disabled_date.daysTo(m_current->date());
			m_progress[pos] = Progress(disabled_date);
			int row = pos / 7;
			int col = pos % 7;
			QModelIndex index = createIndex(row, col);
			emit dataChanged(index, index);
		}
		m_file->setValue(QLatin1String("HistoryDisabled"), m_current->date().toString(Qt::ISODate));
	}
	m_file->beginGroup(QLatin1String("Progress"));

	// Load goal
	m_type = Preferences::instance().goalType();
	if (m_type == 1) {
		m_goal = Preferences::instance().goalMinutes() * 60000;
	} else if (m_type == 2) {
		m_goal = Preferences::instance().goalWords();
	} else {
		m_goal = 0;
	}

	// Refresh current value if visible
	m_current_valid = false;
	updateProgress();

	// Refresh streaks if minimum percent has changed
	int streak_minimum = m_streak_minimum;
	m_streak_minimum = Preferences::instance().goalStreakMinimum();
	if (streak_minimum != m_streak_minimum) {
		emit streaksChanged();
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::resetToday()
{
	m_words = 0;
	m_msecs = 0;
	m_current_valid = false;
	updateProgress();
}

//-----------------------------------------------------------------------------

int DailyProgress::columnCount(const QModelIndex&) const
{
	return 9;
}

//-----------------------------------------------------------------------------

QVariant DailyProgress::data(const QModelIndex& index, int role) const
{
	QVariant result;

	int column = index.column() - 1;
	if (column == -1) {
		switch (role) {
		case Qt::DisplayRole:
			result = m_row_month_names.value(index.row());
			break;

		case Qt::TextAlignmentRole:
			result = int(Qt::AlignRight | Qt::AlignVCenter);
			break;

		default:
			break;
		}
		return result;
	} else if (column == 7) {
		switch (role) {
		case Qt::DisplayRole:
			result = m_row_year_names.value(index.row());
			break;

		case Qt::TextAlignmentRole:
			result = int(Qt::AlignLeft | Qt::AlignVCenter);
			break;

		default:
			break;
		}
		return result;
	}

	Progress progress = m_progress.value((index.row() * 7) + column);
	if (!progress.date().isValid()) {
		return result;
	}

	switch (role) {
	case Qt::DisplayRole:
		result = QString::number(progress.date().day());
		break;

	case Qt::TextAlignmentRole:
		result = Qt::AlignCenter;
		break;

	case Qt::UserRole:
		result = progress.progress();
		break;

	case Qt::ToolTipRole:
		result = QString("<center><small><b>%1</b></small><br>%2</center>")
				.arg(progress.date().toString(Qt::DefaultLocaleLongDate))
				.arg(progress.progressString());
		break;

	default:
		break;
	}

	return result;
}

//-----------------------------------------------------------------------------

Qt::ItemFlags DailyProgress::flags(const QModelIndex&) const
{
	return Qt::NoItemFlags;
}

//-----------------------------------------------------------------------------

QVariant DailyProgress::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal) {
		if (role == Qt::DisplayRole) {
			return m_day_names[section];
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------

int DailyProgress::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : std::ceil(m_progress.size() / 7.0);
}

//-----------------------------------------------------------------------------

void DailyProgress::save()
{
	m_file->setValue(m_current->date().toString(Qt::ISODate), QVariantList()
			<< m_words
			<< m_msecs
			<< m_type
			<< m_goal);
}

//-----------------------------------------------------------------------------

void DailyProgress::setProgressEnabled(bool enable)
{
	if (enable) {
		++m_progress_enabled;
		updateProgress();
	} else if (m_progress_enabled) {
		--m_progress_enabled;
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------

void DailyProgress::updateDay()
{
	if (m_current->date() == QDate::currentDate()) {
		return;
	}

	// Store current progress
	if (Preferences::instance().goalHistory()) {
		save();
	} else {
		m_words = 0;
		m_msecs = 0;
		m_current->setProgress(m_words, m_msecs, m_type, m_goal);
		m_file->remove(m_current->date().toString(Qt::ISODate));
		m_file->setValue(QLatin1String("HistoryDisabled"), QDate::currentDate().toString(Qt::ISODate));
	}

	// Make sure all days are accounted for
	updateRows();

	// Reset current progress
	m_words = 0;
	m_msecs = 0;
	m_current_valid = false;
	updateProgress();
}

//-----------------------------------------------------------------------------

void DailyProgress::findStreak(int pos, int& start, int& end) const
{
	start = end = -1;
	for (int i = pos; i >= 0; --i) {
		if (m_progress.at(i).progress() >= m_streak_minimum) {
			start = i;
			if (end == -1) {
				end = i;
			}
		} else {
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::updateProgress()
{
	if (m_progress_enabled) {
		percentComplete();
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::updateRows()
{
	beginResetModel();

	// Make sure current date exists
	const QDate date = QDate::currentDate();
	const Progress& progress = m_progress.last();
	if (progress.date() != date) {
		const int type = progress.type();
		const int goal = progress.goal();
		m_progress.append(Progress(date, 0, 0, type, goal));
	}

	// Add empty entries for days without data
	QDate previous = m_progress.last().date();
	for (int i = m_progress.size() - 2; i >= 0; --i) {
		const Progress& progress = m_progress.at(i);
		const QDate next = progress.date();
		const int type = progress.type();
		const int goal = progress.goal();
		const int count = next.daysTo(previous) - 1;
		for (int j = 0; j < count; ++j) {
			previous = previous.addDays(-1);
			m_progress.insert(i + 1, Progress(previous, 0, 0, type, goal));
		}
		previous = next;
	}

	// Fetch current daily progress
	m_current_pos = m_progress.size() - 1;
	m_current = &m_progress[m_current_pos];

	// Fetch row month and year names
	QLocale locale;
	int month = -1;
	int year = -1;
	int offset = 0;
	for (int i = 0; i < m_progress.size(); i += 7) {
		const Progress& progress = m_progress.at(i);
		if (progress.date().isNull()) {
			i -= 6;
			++offset;
			continue;
		} else if (offset) {
			i -= offset;
			offset = 0;
		}

		const int row = i / 7;
		if (progress.date().month() != month) {
			month = progress.date().month();
			const QString name = locale.standaloneMonthName(progress.date().month(), QLocale::ShortFormat);
			m_row_month_names.insert(row, name);
		}
		if (progress.date().year() != year) {
			year = progress.date().year();
			const QString name = QString::number(year);
			m_row_year_names.insert(row, name);
		}
	}

	endResetModel();
}

//-----------------------------------------------------------------------------

QString DailyProgress::Progress::progressString() const
{
	if (m_type == 1) {
		return DailyProgress::tr("%L1% of %Ln minute(s)", "", m_goal / 60000).arg(m_progress);
	} else if (m_type == 2) {
		return DailyProgress::tr("%L1% of %Ln word(s)", "", m_goal).arg(m_progress);
	} else if (m_words) {
		return DailyProgress::tr("%Ln word(s)", "", m_words);
	} else if (m_msecs) {
		return DailyProgress::tr("%Ln minute(s)", "", m_msecs / 60000);
	} else {
		return DailyProgress::tr("0%");
	}
}

//-----------------------------------------------------------------------------

void DailyProgress::Progress::setProgress(int words, int msecs, int type, int goal)
{
	m_words = words;
	m_msecs = msecs;
	m_type = type;
	m_goal = goal;
	calculateProgress();
}

//-----------------------------------------------------------------------------

void DailyProgress::Progress::calculateProgress()
{
	m_progress = 0;
	if (m_goal > 0) {
		if (m_type == 1) {
			m_progress = (m_msecs * 100) / m_goal;
		} else if (m_type == 2) {
			m_progress = (m_words * 100) / m_goal;
		}
	} else if (m_words || m_msecs) {
		m_progress = 100;
	}
}

//-----------------------------------------------------------------------------
