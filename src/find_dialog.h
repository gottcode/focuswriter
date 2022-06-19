/*
	SPDX-FileCopyrightText: 2008-2012 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_FIND_DIALOG_H
#define FOCUSWRITER_FIND_DIALOG_H

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
	explicit FindDialog(Stack* documents);

	bool eventFilter(QObject* watched, QEvent* event) override;

public Q_SLOTS:
	void findNext();
	void findPrevious();
	void reject() override;
	void showFindMode();
	void showReplaceMode();

Q_SIGNALS:
	void findNextAvailable(bool available);

protected:
	void moveEvent(QMoveEvent* event) override;
	void showEvent(QShowEvent* event) override;

private Q_SLOTS:
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

#endif // FOCUSWRITER_FIND_DIALOG_H
