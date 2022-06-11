/*
	SPDX-FileCopyrightText: 2010-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_TIMER_H
#define FOCUSWRITER_TIMER_H

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
	explicit Timer(const Stack* documents, QWidget* parent = nullptr);
	Timer(int type, const QStringList& values, const Stack* documents, QWidget* parent = nullptr);
	Timer(const QString& id, const Stack* documents, QWidget* parent = nullptr);
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

public Q_SLOTS:
	void documentAdded(const Document* document);
	void documentRemoved(const Document* document);

Q_SIGNALS:
	void changed(Timer* timer);
	void edited(Timer* timer);

private Q_SLOTS:
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

	const Stack* m_documents;
	QTimer* m_timer;

	QHash<const Document*, Deltas*> m_deltas;
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

#endif // FOCUSWRITER_TIMER_H
