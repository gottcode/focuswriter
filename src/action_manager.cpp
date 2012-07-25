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

#include "action_manager.h"

#include <QAction>
#include <QSettings>

//-----------------------------------------------------------------------------

ActionManager* ActionManager::m_instance = 0;

//-----------------------------------------------------------------------------

ActionManager::ActionManager(QObject* parent) :
	QObject(parent)
{
	m_instance = this;

	// Load shortcuts
	QSettings settings;
	settings.beginGroup("Shortcuts");
	QStringList keys = settings.childKeys();
	foreach (const QString& name, keys) {
		m_actions[name].shortcut = settings.value(name).value<QKeySequence>();
	}
}

//-----------------------------------------------------------------------------

ActionManager::~ActionManager()
{
	if (m_instance == this) {
		m_instance = 0;
	}
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

void ActionManager::addAction(const QString& name, QAction* action)
{
	Action& act = m_actions[name];
	act.action = action;
	act.default_shortcut = action->shortcut();
	if (!act.shortcut.isEmpty()) {
		action->setShortcut(act.shortcut);
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
		if (!act.shortcut.isEmpty() && (act.default_shortcut != act.shortcut)) {
			settings.setValue(name, act.shortcut);
		} else {
			settings.remove(name);
		}
	}
}

//-----------------------------------------------------------------------------
