/*
	SPDX-FileCopyrightText: 2009-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef SPELL_H
#define SPELL_H

class DictionaryRef;

#include <QDialog>
#include <QTextCursor>
class QAction;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QTextEdit;

class SpellChecker : public QDialog
{
	Q_OBJECT

public:
	static void checkDocument(QTextEdit* document, DictionaryRef& dictionary);

public slots:
	virtual void reject();

private slots:
	void suggestionChanged(QListWidgetItem* suggestion);
	void add();
	void ignore();
	void ignoreAll();
	void change();
	void changeAll();

private:
	SpellChecker(QTextEdit* document, DictionaryRef& dictionary);
	void check();

private:
	DictionaryRef& m_dictionary;

	QTextEdit* m_document;
	QTextEdit* m_context;
	QLineEdit* m_suggestion;
	QListWidget* m_suggestions;
	QTextCursor m_cursor;
	QTextCursor m_start_cursor;

	int m_checked_blocks;
	int m_total_blocks;
	bool m_loop_available;

	QString m_word;
	QStringList m_ignored;
};

#endif
