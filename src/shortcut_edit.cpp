/***********************************************************************
 *
 * Copyright (C) 2012, 2014 Graeme Gott <graeme@gottcode.org>
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
#include <QPushButton>

#if (QT_VERSION >= (QT_VERSION_CHECK(5,2,0)))
#include <QKeySequenceEdit>
#else

#include <QLineEdit>

class QKeySequenceEdit : public QLineEdit
{
public:
	QKeySequenceEdit(QWidget* parent) :
		QLineEdit(parent)
	{
	}

	void clear()
	{
		setKeySequence(QKeySequence());
	}

	QKeySequence keySequence() const
	{
		return m_sequence;
	}

	void setKeySequence(const QKeySequence& sequence)
	{
		m_sequence = sequence;
		setText(m_sequence.toString(QKeySequence::NativeText));
	}

protected:
	void keyPressEvent(QKeyEvent* event);

private:
	QKeySequence m_sequence;
};

void QKeySequenceEdit::keyPressEvent(QKeyEvent* event)
{
	Qt::KeyboardModifiers modifiers = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
	int key = event->key();

	switch (key) {
	// Don't do anything if they only press a modifier
	case Qt::Key_Shift:
	case Qt::Key_Control:
	case Qt::Key_Meta:
	case Qt::Key_Alt:
		return;

	// Clear on backspace unless modifier is used
	case Qt::Key_Backspace:
	case Qt::Key_Delete:
		if (modifiers == Qt::NoModifier) {
			clear();
			return;
		}
		break;

	// Allow tab to change focus
	case Qt::Key_Tab:
	case Qt::Key_Backtab:
		return;

	default:
		break;
	}

	// Add modifiers; only allow shift if it is not required for key of shortcut
	if (modifiers & Qt::ShiftModifier) {
		QChar c = !event->text().isEmpty() ? event->text().at(0) : QChar();
		if (!c.isPrint() || c.isLetterOrNumber() || c.isSpace()) {
			key |= Qt::SHIFT;
		}
	}
	if (modifiers & Qt::ControlModifier) {
		key |= Qt::CTRL;
	}
	if (modifiers & Qt::MetaModifier) {
		key |= Qt::META;
	}
	if (modifiers & Qt::AltModifier) {
		key |= Qt::ALT;
	}

	// Change shortcut
	setKeySequence(QKeySequence(key));
}

#endif

//-----------------------------------------------------------------------------

ShortcutEdit::ShortcutEdit(QWidget* parent) :
	QWidget(parent)
{
	m_edit = new QKeySequenceEdit(this);
	connect(m_edit, SIGNAL(editingFinished()), this, SIGNAL(changed()));

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
	return m_edit->keySequence();
}

//-----------------------------------------------------------------------------

void ShortcutEdit::setShortcut(const QKeySequence& shortcut, const QKeySequence& default_shortcut)
{
	m_default_shortcut = default_shortcut;
	m_reset_button->setHidden(default_shortcut.isEmpty());
	m_edit->setKeySequence(shortcut);
}

//-----------------------------------------------------------------------------

void ShortcutEdit::clear()
{
	m_edit->clear();
	emit changed();
}

//-----------------------------------------------------------------------------

void ShortcutEdit::reset()
{
	m_edit->setKeySequence(m_default_shortcut);
	emit changed();
}

//-----------------------------------------------------------------------------
