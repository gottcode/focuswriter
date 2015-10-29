/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2015 Graeme Gott <graeme@gottcode.org>
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

#include <QAbstractTableModel>
#include <QDate>
#include <QStringList>
#include <QElapsedTimer>
#include <QVector>
class QSettings;

class DailyProgress : public QAbstractTableModel
{
	Q_OBJECT

public:
	DailyProgress(QObject* parent = 0);
	~DailyProgress();

	void findCurrentStreak(QDate& start, QDate& end) const;
	void findLongestStreak(QDate& start, QDate& end) const;
	int percentComplete();

	void increaseWordCount(int words);
	void increaseTime();
	void loadPreferences();
	void resetToday();

	int columnCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex& parent = QModelIndex()) const;

	static void setPath(const QString& path);

public slots:
	void save();
	void setProgressEnabled(bool enable = true);

signals:
	void progressChanged();
	void streaksChanged();

private slots:
	void updateDay();

private:
	void findStreak(int pos, int& start, int& end) const;
	void updateProgress();
	void updateRows();

private:
	QSettings* m_file;

	int m_words;
	int m_msecs;
	int m_type;
	int m_goal;

	QElapsedTimer m_typing_timer;

	class Progress
	{
	public:
		Progress(const QDate& date = QDate()) :
			m_date(date), m_words(0), m_msecs(0), m_type(0), m_goal(0), m_progress(0)
			{ }

		Progress(const QDate& date, int words, int msecs, int type, int goal) :
			m_date(date), m_words(words), m_msecs(msecs), m_type(type), m_goal(goal), m_progress(0)
			{ calculateProgress(); }

		QDate date() const
			{ return m_date; }

		int goal() const
			{ return m_goal; }

		int type() const
			{ return m_type; }

		int progress() const
			{ return m_progress; }

		QString progressString() const;

		void setDate(const QDate& date)
			{ m_date = date; }

		void setProgress(int words, int msecs, int type, int goal);

	private:
		void calculateProgress();

	private:
		QDate m_date;
		int m_words;
		int m_msecs;
		int m_type;
		int m_goal;
		int m_progress;
	};
	QVector<Progress> m_progress;
	Progress* m_current;
	bool m_current_valid;
	int m_current_pos;
	int m_progress_enabled;
	int m_streak_minimum;

	QStringList m_day_names;
	QHash<int, QString> m_row_month_names;
	QHash<int, QString> m_row_year_names;

	static QString m_path;
};

inline void DailyProgress::increaseWordCount(int words)
{
	m_words += words;
	m_current_valid = false;
	updateProgress();
}

#endif
