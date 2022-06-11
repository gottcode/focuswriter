/*
	SPDX-FileCopyrightText: 2012-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SHORTCUT_EDIT_H
#define FOCUSWRITER_SHORTCUT_EDIT_H

#include <QWidget>
class QKeySequenceEdit;
class QPushButton;

class ShortcutEdit : public QWidget
{
	Q_OBJECT

public:
	explicit ShortcutEdit(QWidget* parent = nullptr);

	QKeySequence shortcut() const;

	void setShortcut(const QKeySequence& shortcut, const QKeySequence& default_shortcut = QKeySequence());

Q_SIGNALS:
	void changed();

public Q_SLOTS:
	void clear();
	void reset();

private:
	QKeySequence m_default_shortcut;
	QKeySequenceEdit* m_edit;
	QPushButton* m_reset_button;
};

#endif // FOCUSWRITER_SHORTCUT_EDIT_H
