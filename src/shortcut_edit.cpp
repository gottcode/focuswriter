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

#include "shortcut_edit.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

//-----------------------------------------------------------------------------

ShortcutEdit::ShortcutEdit(QWidget* parent) :
	QWidget(parent)
{
	m_edit = new QLineEdit(this);
	m_edit->installEventFilter(this);

	QPushButton* clear_button = new QPushButton(tr("Clear"), this);
	connect(clear_button, SIGNAL(clicked()), this, SLOT(clear()));

	m_reset_button = new QPushButton(tr("Reset to Default"), this);
	connect(m_reset_button, SIGNAL(clicked()), this, SLOT(reset()));
	m_reset_button->hide();

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(m_edit);
	layout->addWidget(clear_button);
	layout->addWidget(m_reset_button);

	setFocusPolicy(Qt::WheelFocus);
	setFocusProxy(m_edit);
}

//-----------------------------------------------------------------------------

QKeySequence ShortcutEdit::shortcut() const
{
	return m_shortcut;
}

//-----------------------------------------------------------------------------

bool ShortcutEdit::eventFilter(QObject* watched, QEvent* event)
{
	if ((watched == m_edit) && (event->type() == QEvent::KeyPress)) {
		QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

		Qt::KeyboardModifiers modifiers = key_event->modifiers();
		int key = key_event->key();

		switch (key) {
		// Don't do anything if they only press a modifier
		case Qt::Key_Shift:
		case Qt::Key_Control:
		case Qt::Key_Meta:
		case Qt::Key_Alt:
			return true;

		// Clear on backspace unless modifier is used
		case Qt::Key_Backspace:
		case Qt::Key_Delete:
			if (modifiers == Qt::NoModifier) {
				clear();
				return true;
			}
			break;

		// Allow tab to change focus
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			return false;

		default:
			break;
		}

		// Prevent shortcuts from intefering with typing
		if ((modifiers == Qt::NoModifier) && ((key < Qt::Key_F1) || (key > Qt::Key_F35)) && (key != Qt::Key_Escape)) {
			return true;
		}

		// Only allow shift when it is not required for key of shortcut
		if ((modifiers & Qt::ShiftModifier) && !key_event->text().isEmpty()) {
			QChar c = key_event->text().at(0);
			if (c.isPrint() && !c.isLetterOrNumber() && !c.isSpace()) {
				modifiers &= ~Qt::ShiftModifier;
			}
		}

		// Change shortcut
		m_shortcut = QKeySequence(key | modifiers);
		setText();
		emit changed();

		return true;
	} else {
		return QWidget::eventFilter(watched, event);
	}
}

//-----------------------------------------------------------------------------

void ShortcutEdit::setShortcut(const QKeySequence& shortcut, const QKeySequence& default_shortcut)
{
	m_shortcut = shortcut;
	m_default_shortcut = default_shortcut;
	m_reset_button->setHidden(m_default_shortcut.isEmpty());
	setText();
}

//-----------------------------------------------------------------------------

void ShortcutEdit::clear()
{
	m_shortcut = QKeySequence();
	setText();
	emit changed();
}

//-----------------------------------------------------------------------------

void ShortcutEdit::reset()
{
	m_shortcut = m_default_shortcut;
	setText();
	emit changed();
}

//-----------------------------------------------------------------------------

void ShortcutEdit::setText()
{
	m_edit->setText(m_shortcut.toString(QKeySequence::NativeText));
}

//-----------------------------------------------------------------------------
