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

#include <QFile>
#include <QLocale>
#include <QSettings>

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
	m_current_pos(0)
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
		foreach (const QString& key, keys) {
			QDate date = QDate::fromString(key, Qt::ISODate);
			if (!date.isValid()) {
				continue;
			}
			Progress progress(date);
			QVariantList values = m_file->value(key).toList();
			if (values.size() == 4) {
				progress.setProgress(values.at(0).toInt(), values.at(1).toInt(), values.at(2).toInt(), values.at(3).toInt());
			}
			m_progress.append(progress);
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

	// Add empty entries for days without data
	QDate previous = m_progress.last().date();
	for (int i = m_progress.size() - 2; i >= 0; --i) {
		QDate next = m_progress.at(i).date();
		int count = next.daysTo(previous) - 1;
		for (int j = 0; j < count; ++j) {
			previous = previous.addDays(-1);
			m_progress.insert(i + 1, Progress(previous));
		}
		previous = next;
	}

	// Add null entries before data to make it week-based
	QLocale locale;
#if (QT_VERSION >= (QT_VERSION_CHECK(4,8,0)))
	int start_of_week = locale.firstDayOfWeek();
#else
	int start_of_week = Qt::Sunday;
#endif
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

	// Add null entries after data to make it week-based
	null_days = 7 - (m_progress.size() % 7);
	if (null_days < 7) {
		for (int i = 0; i < null_days; ++i) {
			m_progress.append(Progress());
		}
	}

	// Fetch current daily progress
	for (m_current_pos = m_progress.size() - 1; m_current_pos >= 0; --m_current_pos) {
		if (m_progress.at(m_current_pos).date() == date) {
			m_current = &m_progress[m_current_pos];
			break;
		}
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

	m_typing_timer.start();
}

//-----------------------------------------------------------------------------

DailyProgress::~DailyProgress()
{
	save();
}

//-----------------------------------------------------------------------------

int DailyProgress::percentComplete()
{
	if (!m_current_valid) {
		m_current_valid = true;
		m_current->setProgress(m_words, m_msecs, m_type, m_goal);
		QModelIndex index = createIndex(m_current_pos / 7, m_current_pos % 7);
		emit dataChanged(index, index);
	}
	return m_current->progress();
}

//-----------------------------------------------------------------------------

void DailyProgress::increaseTime()
{
	qint64 msecs = m_typing_timer.restart();
	if (msecs < 30000) {
		m_msecs += msecs;
		m_current_valid = false;
	}
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
	m_current_valid = false;
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
	if ((column == -1) || (column == 7)) {
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
		result = QString("<center><small><b>%1</b></small><br>%2%</center>")
				.arg(progress.date().toString(Qt::DefaultLocaleLongDate))
				.arg(progress.progress());
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
	return parent.isValid() ? 0 : (m_progress.size() / 7);
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

void DailyProgress::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------

void DailyProgress::Progress::setProgress(int words, int msecs, int type, int goal)
{
	m_progress = 0;
	if (goal > 0) {
		if (type == 1) {
			m_progress = (msecs * 100) / goal;
		} else if (type == 2) {
			m_progress = (words * 100) / goal;
		}
	} else if (words || msecs) {
		m_progress = 100;
	}
}

//-----------------------------------------------------------------------------
