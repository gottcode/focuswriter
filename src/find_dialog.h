/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2012 Graeme Gott <graeme@gottcode.org>
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

#ifndef FIND_DIALOG_H
#define FIND_DIALOG_H

class Stack;

#include <QDialog>
class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;

class FindDialog : public QDialog
{
	Q_OBJECT

public:
	FindDialog(Stack* documents);

	virtual bool eventFilter(QObject* watched, QEvent* event);

public slots:
	void findNext();
	void findPrevious();
	void reject();
	void showFindMode();
	void showReplaceMode();

signals:
	void findNextAvailable(bool available);

protected:
	void moveEvent(QMoveEvent* event);
	void showEvent(QShowEvent* event);

private slots:
	void find();
	void findChanged(const QString& text);
	void replace();
	void replaceAll();

private:
	void find(bool backwards);
	void showMode(bool replace);

private:
	Stack* m_documents;

	QLineEdit* m_find_string;
	QLabel* m_replace_label;
	QLineEdit* m_replace_string;

	QCheckBox* m_ignore_case;
	QCheckBox* m_whole_words;
	QCheckBox* m_regular_expressions;
	QRadioButton* m_search_backwards;

	QPushButton* m_find_button;
	QPushButton* m_replace_button;
	QPushButton* m_replace_all_button;

	QPoint m_position;
};

#endif
