/*
	SPDX-FileCopyrightText: 2012-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef SHORTCUT_EDIT_H
#define SHORTCUT_EDIT_H

#include <QWidget>
class QKeySequenceEdit;
class QPushButton;

class ShortcutEdit : public QWidget
{
	Q_OBJECT

public:
	ShortcutEdit(QWidget* parent = 0);

	QKeySequence shortcut() const;

	void setShortcut(const QKeySequence& shortcut, const QKeySequence& default_shortcut = QKeySequence());

signals:
	void changed();

public slots:
	void clear();
	void reset();

private:
	QKeySequence m_default_shortcut;
	QKeySequenceEdit* m_edit;
	QPushButton* m_reset_button;
};

#endif
