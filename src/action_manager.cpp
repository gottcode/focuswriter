/*
	SPDX-FileCopyrightText: 2012 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "action_manager.h"

#include <QAction>
#include <QSettings>
#include <QShortcut>
#include <QVariantHash>
#include <QWidget>

//-----------------------------------------------------------------------------

ActionManager* ActionManager::m_instance = nullptr;

//-----------------------------------------------------------------------------

ActionManager::ActionManager(QWidget* parent)
	: QObject(parent)
	, m_widget(parent)
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
	const QStringList keys = settings.childKeys();
	for (const QString& name : keys) {
		m_actions[name].shortcut = settings.value(name).value<QKeySequence>();
	}

	// Load symbol shortcuts
	static const QVariantHash defshortcuts{
		{ "2014", "Ctrl+-" },
		{ "2019", "Ctrl+Shift+=" },
		{ "2022", "Ctrl+*" },
		{ "2026", "Ctrl+." }
	};
	QVariantHash shortcuts = QSettings().value("SymbolsDialog/Shortcuts", defshortcuts).toHash();
	for (auto defiter = defshortcuts.cbegin(); defiter != defshortcuts.cend(); ++defiter) {
		if (!shortcuts.contains(defiter.key()) && shortcuts.key(defiter.value()).isEmpty()) {
			shortcuts.insert(defiter.key(), defiter.value());
		}
	}
	for (auto iter = shortcuts.cbegin(); iter != shortcuts.cend(); ++iter) {
		bool ok = false;
		const char32_t unicode = iter.key().toUInt(&ok, 16);
		const QKeySequence shortcut = QKeySequence::fromString(iter.value().toString());
		if (ok && !shortcut.isEmpty()) {
			addShortcut(unicode, shortcut);
		}
	}
}

//-----------------------------------------------------------------------------

ActionManager::~ActionManager()
{
	if (m_instance == this) {
		m_instance = nullptr;
	}

	// Save symbol shortcuts
	QVariantHash shortcuts;
	for (auto iter = m_symbol_shortcuts.cbegin(); iter != m_symbol_shortcuts.cend(); ++iter) {
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

QKeySequence ActionManager::shortcut(char32_t unicode) const
{
	return m_symbol_shortcuts.contains(unicode) ? m_symbol_shortcuts.value(unicode)->key() : QKeySequence();
}

//-----------------------------------------------------------------------------

void ActionManager::addAction(const QString& name, QAction* action)
{
	const bool set = m_actions.contains(name);
	Action& act = m_actions[name];
	act.action = action;
	act.default_shortcut = action->shortcut();
	if (!set) {
		act.shortcut = act.default_shortcut;
	}
	action->setShortcut(act.shortcut);
}

//-----------------------------------------------------------------------------

void ActionManager::setShortcut(char32_t unicode, const QKeySequence& sequence)
{
	if (!sequence.isEmpty()) {
		if (m_symbol_shortcuts.contains(unicode)) {
			m_symbol_shortcuts.value(unicode)->setKey(sequence);
		} else {
			addShortcut(unicode, sequence);
		}
	} else {
		const QShortcut* shortcut = m_symbol_shortcuts.value(unicode);
		m_symbol_shortcuts.remove(unicode);
		m_symbol_shortcuts_text.remove(shortcut);
		delete shortcut;
	}
}

//-----------------------------------------------------------------------------

void ActionManager::setShortcuts(const QHash<QString, QKeySequence>& shortcuts)
{
	// Update specified shortcuts
	for (auto i = shortcuts.cbegin(); i != shortcuts.cend(); ++i) {
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
	for (auto i = m_actions.cbegin(); i != m_actions.cend(); ++i) {
		const QString& name = i.key();
		const Action& act = i.value();
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
	const QObject* object = sender();
	if (m_symbol_shortcuts_text.contains(object)) {
		Q_EMIT insertText(m_symbol_shortcuts_text.value(object));
	}
}

//-----------------------------------------------------------------------------

void ActionManager::addShortcut(char32_t unicode, const QKeySequence& sequence)
{
	QShortcut* shortcut = new QShortcut(sequence, m_widget);
	connect(shortcut, &QShortcut::activated, this, &ActionManager::symbolShortcutActivated);
	m_symbol_shortcuts[unicode] = shortcut;
	m_symbol_shortcuts_text[shortcut] = QString::fromUcs4(&unicode, 1);
}

//-----------------------------------------------------------------------------
