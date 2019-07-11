/***********************************************************************
 *
 * Copyright (C) 2010, 2019 Graeme Gott <graeme@gottcode.org>
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

#ifndef TIMER_H
#define TIMER_H

class Document;
class Deltas;
class Stack;

#include <QDateTime>
#include <QFrame>
#include <QHash>
#include <QTime>
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimeEdit;
class QTimer;

class Timer : public QFrame
{
	Q_OBJECT

public:
	Timer(Stack* documents, QWidget* parent = 0);
	Timer(int type, const QStringList& values, Stack* documents, QWidget* parent = 0);
	Timer(const QString& id, Stack* documents, QWidget* parent = 0);
	~Timer();

	bool isEditing() const;
	bool isRunning() const;
	QString memo() const;
	QString memoShort() const;
	int msecsFrom(const QDateTime& start) const;
	int msecsTotal() const;

	void cancelEditing();
	void save();

	bool operator<=(const Timer& timer) const;

	static QString toString(const QString& time, const QString& memo);

public slots:
	void documentAdded(Document* document);
	void documentRemoved(Document* document);

signals:
	void changed(Timer* timer);
	void edited(Timer* timer);

private slots:
	void delayChanged(const QTime& delay);
	void endChanged(const QTime& end);
	void editAccepted();
	void editRejected();
	void editClicked();
	void removeClicked();
	void timerFinished();

private:
	void init();
	void remove();
	void setMode(bool edit);
	bool startTimer();
	void updateCounts();

private:
	QString m_id;
	QDateTime m_start;
	QDateTime m_end;
	QTime m_delay;
	int m_delay_msecs;
	QString m_memo;
	QString m_memo_short;
	int m_type;
	bool m_started;
	bool m_finished;

	Stack* m_documents;
	QTimer* m_timer;

	QHash<Document*, Deltas*> m_deltas;
	int m_character_count;
	int m_character_and_space_count;
	int m_page_count;
	int m_paragraph_count;
	int m_word_count;

	// Edit widgets
	QWidget* m_edit;
	QComboBox* m_type_box;
	QTimeEdit* m_end_edit;
	QTimeEdit* m_delay_edit;
	QLineEdit* m_memo_edit;
	QPushButton* m_ok_button;

	// Display widgets
	QWidget* m_display;
	QLabel* m_display_label;
	QPushButton* m_edit_button;
};

#endif
