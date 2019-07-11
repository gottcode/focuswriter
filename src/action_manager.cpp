/***********************************************************************
 *
 * Copyright (C) 2012, 2014, 2015, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "action_manager.h"

#include <QAction>
#include <QSettings>
#include <QShortcut>
#include <QVariantHash>

//-----------------------------------------------------------------------------

ActionManager* ActionManager::m_instance = 0;

//-----------------------------------------------------------------------------

ActionManager::ActionManager(QWidget* parent) :
	QObject(parent),
	m_widget(parent)
{
	m_instance = this;

#ifdef Q_OS_MAC
	// Override default shortcuts to prevent conflicts
	m_actions["SwitchNextDocument"].shortcut = Qt::CTRL | Qt::Key_BracketRight;
	m_actions["SwitchPreviousDocument"].shortcut = Qt::CTRL | Qt::Key_BracketLeft;
	m_actions["Reload"].shortcut = Qt::Key_F5;
#endif

	// Load shortcuts
	QSettings settings;
	settings.beginGroup("Shortcuts");
	QStringList keys = settings.childKeys();
	for (const QString& name : keys) {
		m_actions[name].shortcut = settings.value(name).value<QKeySequence>();
	}

	// Load symbol shortcuts
	QVariantHash defshortcuts;
	defshortcuts.insert("2014", "Ctrl+-");
	defshortcuts.insert("2019", "Ctrl+Shift+=");
	defshortcuts.insert("2022", "Ctrl+*");
	defshortcuts.insert("2026", "Ctrl+.");
	QVariantHash shortcuts = QSettings().value("SymbolsDialog/Shortcuts", defshortcuts).toHash();
	QHashIterator<QString, QVariant> defiter(defshortcuts);
	while (defiter.hasNext()) {
		defiter.next();
		if (!shortcuts.contains(defiter.key())) {
			shortcuts.insert(defiter.key(), defiter.value());
		}
	}
	QHashIterator<QString, QVariant> iter(shortcuts);
	while (iter.hasNext()) {
		iter.next();
		bool ok = false;
		quint32 unicode = iter.key().toUInt(&ok, 16);
		QKeySequence shortcut = QKeySequence::fromString(iter.value().toString());
		if (ok && !shortcut.isEmpty()) {
			addShortcut(unicode, shortcut);
		}
	}
}

//-----------------------------------------------------------------------------

ActionManager::~ActionManager()
{
	if (m_instance == this) {
		m_instance = 0;
	}

	// Save symbol shortcuts
	QVariantHash shortcuts;
	QHashIterator<quint32, QShortcut*> iter(m_symbol_shortcuts);
	while (iter.hasNext()) {
		iter.next();
		shortcuts.insert(QString::number(iter.key(), 16), iter.value()->key().toString());
	}
	QSettings().setValue("SymbolsDialog/Shortcuts", shortcuts);
}

//-----------------------------------------------------------------------------

QKeySequence ActionManager::shortcut(const QString& name) const
{
	if (!m_actions.contains(name)) {
		return QKeySequence();
	}
	const Action& act = m_actions[name];
	return act.action ? act.action->shortcut() : act.shortcut;
}

//-----------------------------------------------------------------------------

QKeySequence ActionManager::shortcut(quint32 unicode)
{
	return m_symbol_shortcuts.contains(unicode) ? m_symbol_shortcuts.value(unicode)->key() : QKeySequence();
}

//-----------------------------------------------------------------------------

void ActionManager::addAction(const QString& name, QAction* action)
{
	bool set = m_actions.contains(name);
	Action& act = m_actions[name];
	act.action = action;
	act.default_shortcut = action->shortcut();
	if (!set) {
		act.shortcut = act.default_shortcut;
	}
	action->setShortcut(act.shortcut);
}

//-----------------------------------------------------------------------------

void ActionManager::setShortcut(quint32 unicode, const QKeySequence& sequence)
{
	if (!sequence.isEmpty()) {
		if (m_symbol_shortcuts.contains(unicode)) {
			m_symbol_shortcuts.value(unicode)->setKey(sequence);
		} else {
			addShortcut(unicode, sequence);
		}
	} else {
		QShortcut* shortcut = m_symbol_shortcuts.value(unicode);
		m_symbol_shortcuts.remove(unicode);
		m_symbol_shortcuts_text.remove(shortcut);
		delete shortcut;
	}
}

//-----------------------------------------------------------------------------

void ActionManager::setShortcuts(const QHash<QString, QKeySequence>& shortcuts)
{
	// Update specified shortcuts
	QHashIterator<QString, QKeySequence> i(shortcuts);
	while (i.hasNext()) {
		i.next();
		const QString& name = i.key();
		if (!m_actions.contains(name)) {
			continue;
		}

		const QKeySequence& shortcut = i.value();
		m_actions[name].action->setShortcut(shortcut);
		m_actions[name].shortcut = shortcut;
	}

	// Save all non-default shortcuts
	QSettings settings;
	settings.beginGroup("Shortcuts");
	QHashIterator<QString, Action> j(m_actions);
	while (j.hasNext()) {
		const QString& name = j.next().key();
		const Action& act = j.value();
		if (act.default_shortcut != act.shortcut) {
			settings.setValue(name, act.shortcut);
		} else {
			settings.remove(name);
		}
	}
}

//-----------------------------------------------------------------------------

void ActionManager::symbolShortcutActivated()
{
	QObject* object = sender();
	if (m_symbol_shortcuts_text.contains(object)) {
		emit insertText(m_symbol_shortcuts_text.value(object));
	}
}

//-----------------------------------------------------------------------------

void ActionManager::addShortcut(quint32 unicode, const QKeySequence& sequence)
{
	QShortcut* shortcut = new QShortcut(sequence, m_widget);
	connect(shortcut, &QShortcut::activated, this, &ActionManager::symbolShortcutActivated);
	m_symbol_shortcuts[unicode] = shortcut;
	m_symbol_shortcuts_text[shortcut] = QString::fromUcs4(&unicode, 1);
}

//-----------------------------------------------------------------------------
