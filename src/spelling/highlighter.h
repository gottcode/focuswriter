/*
	SPDX-FileCopyrightText: 2009-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_HIGHLIGHTER_H
#define FOCUSWRITER_HIGHLIGHTER_H

class DictionaryRef;

#include <QSyntaxHighlighter>
#include <QTextCursor>
class QAction;
class QTextEdit;
class QTimer;

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	Highlighter(QTextEdit* text, DictionaryRef& dictionary);

	bool enabled() const;
	void setEnabled(bool enabled);
	void setMisspelledColor(const QColor& color);

	virtual bool eventFilter(QObject* watched, QEvent* event);
	virtual void highlightBlock(const QString& text);

public slots:
	void updateSpelling();

private slots:
	void cursorPositionChanged();
	void suggestion(QAction* action);

private:
	DictionaryRef& m_dictionary;
	QTimer* m_spell_timer;
	QTextEdit* m_text;
	QTextCursor m_cursor;
	QTextCursor m_start_cursor;
	bool m_enabled;
	QColor m_misspelled;
	QString m_word;
	QTextBlock m_current;
	bool m_changed;

	QAction* m_add_action;
	QAction* m_check_action;
};

inline bool Highlighter::enabled() const
{
	return m_enabled;
}

#endif // FOCUSWRITER_HIGHLIGHTER_H
