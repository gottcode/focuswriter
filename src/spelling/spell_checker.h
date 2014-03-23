/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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
