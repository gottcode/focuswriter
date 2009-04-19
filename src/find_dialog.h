/***********************************************************************
 *
 * Copyright (C) 2008-2009 Graeme Gott <graeme@gottcode.org>
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

#include <QDialog>
class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QRadioButton;
class QTextCursor;

class FindDialog : public QDialog {
	Q_OBJECT
public:
	FindDialog(QPlainTextEdit* document, QWidget* parent = 0);

protected:
	virtual void hideEvent(QHideEvent* event);
	virtual void showEvent(QShowEvent* event);

private slots:
	void findNext();
	void replaceNext();
	void replaceAll();

private:
	QTextCursor find();

private:
	QPlainTextEdit* m_document;
	QLineEdit* m_find_string;
	QLineEdit* m_replace_string;
	QCheckBox* m_match_case;
	QCheckBox* m_whole_words;
	QRadioButton* m_search_backwards;
};

#endif
