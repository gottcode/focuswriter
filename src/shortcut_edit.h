/***********************************************************************
 *
 * Copyright (C) 2012 Graeme Gott <graeme@gottcode.org>
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

#ifndef SHORTCUT_EDIT_H
#define SHORTCUT_EDIT_H

#include <QWidget>
class QLineEdit;
class QPushButton;

class ShortcutEdit : public QWidget
{
	Q_OBJECT

public:
	ShortcutEdit(QWidget* parent = 0);

	QKeySequence shortcut() const;

	bool eventFilter(QObject* watched, QEvent* event);
	void setShortcut(const QKeySequence& shortcut);
	void setShortcut(const QKeySequence& shortcut, const QKeySequence& default_shortcut);

signals:
	void changed();

public slots:
	void clear();
	void reset();

private:
	void setText();

private:
	QKeySequence m_shortcut;
	QKeySequence m_default_shortcut;
	QLineEdit* m_edit;
	QPushButton* m_reset_button;
};

#endif
