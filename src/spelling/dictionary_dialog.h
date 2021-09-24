/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DICTIONARY_DIALOG
#define DICTIONARY_DIALOG

#include <QDialog>
class QListWidget;

class DictionaryDialog : public QDialog
{
	Q_OBJECT

public:
	DictionaryDialog(QWidget* parent = 0);

public slots:
	void accept();

private:
	QListWidget* m_languages;
};

#endif
