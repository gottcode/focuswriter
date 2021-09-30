/*
	SPDX-FileCopyrightText: 2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DICTIONARY_DIALOG
#define FOCUSWRITER_DICTIONARY_DIALOG

#include <QDialog>
class QListWidget;

class DictionaryDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DictionaryDialog(QWidget* parent = nullptr);

public slots:
	void accept();

private:
	QListWidget* m_languages;
};

#endif // FOCUSWRITER_DICTIONARY_DIALOG
