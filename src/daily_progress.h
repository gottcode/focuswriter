/*
	SPDX-FileCopyrightText: 2013-2015 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DAILY_PROGRESS_H
#define FOCUSWRITER_DAILY_PROGRESS_H

#include <QAbstractTableModel>
#include <QDate>
#include <QStringList>
#include <QElapsedTimer>
class QSettings;

class DailyProgress : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit DailyProgress(QObject* parent = nullptr);
	~DailyProgress();

	void findCurrentStreak(QDate& start, QDate& end) const;
	void findLongestStreak(QDate& start, QDate& end) const;
	int percentComplete();

	void increaseWordCount(int words);
	void increaseTime();
	void loadPreferences();
	void resetToday();

	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	static void setPath(const QString& path);

public Q_SLOTS:
	void save();
	void setProgressEnabled(bool enable = true);

Q_SIGNALS:
	void progressChanged();
	void streaksChanged();

private Q_SLOTS:
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
		Progress(const QDate& date = QDate())
			: m_date(date)
			, m_words(0)
			, m_msecs(0)
			, m_type(0)
			, m_goal(0)
			, m_progress(0)
		{
		}

		Progress(const QDate& date, int words, int msecs, int type, int goal)
			: m_date(date)
			, m_words(words)
			, m_msecs(msecs)
			, m_type(type)
			, m_goal(goal)
			, m_progress(0)
		{
			calculateProgress();
		}

		QDate date() const
		{
			return m_date;
		}

		int goal() const
		{
			return m_goal;
		}

		int type() const
		{
			return m_type;
		}

		int progress() const
		{
			return m_progress;
		}

		QString progressString() const;

		void setDate(const QDate& date)
		{
			m_date = date;
		}

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
	QList<Progress> m_progress;
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

#endif // FOCUSWRITER_DAILY_PROGRESS_H
