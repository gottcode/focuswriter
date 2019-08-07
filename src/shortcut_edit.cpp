/***********************************************************************
 *
 * Copyright (C) 2012, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>

//-----------------------------------------------------------------------------

ShortcutEdit::ShortcutEdit(QWidget* parent) :
	QWidget(parent)
{
	m_edit = new QKeySequenceEdit(this);
	connect(m_edit, &QKeySequenceEdit::editingFinished, this, &ShortcutEdit::changed);

	QPushButton* clear_button = new QPushButton(tr("Clear"), this);
	connect(clear_button, &QPushButton::clicked, this, &ShortcutEdit::clear);

	m_reset_button = new QPushButton(tr("Reset to Default"), this);
	connect(m_reset_button, &QPushButton::clicked, this, &ShortcutEdit::reset);
	m_reset_button->hide();

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
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
