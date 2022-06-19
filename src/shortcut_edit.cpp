/*
	SPDX-FileCopyrightText: 2012-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "shortcut_edit.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>

//-----------------------------------------------------------------------------

ShortcutEdit::ShortcutEdit(QWidget* parent)
	: QWidget(parent)
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
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

void ShortcutEdit::reset()
{
	m_edit->setKeySequence(m_default_shortcut);
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------
