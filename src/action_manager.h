/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_ACTION_MANAGER_H
#define FOCUSWRITER_ACTION_MANAGER_H

#include <QHash>
#include <QKeySequence>
#include <QObject>
#include <QStringList>
class QAction;
class QShortcut;

class ActionManager : public QObject
{
	Q_OBJECT

	struct Action
	{
		QAction* action;
		QKeySequence shortcut;
		QKeySequence default_shortcut;
	};

public:
	explicit ActionManager(QWidget* parent = nullptr);
	~ActionManager();

	QStringList actions() const
	{
		return m_actions.keys();
	}

	QAction* action(const QString& name) const
	{
		return m_actions[name].action;
	}

	QKeySequence defaultShortcut(const QString& name) const
	{
		return m_actions[name].default_shortcut;
	}

	QKeySequence shortcut(const QString& name) const;
	QKeySequence shortcut(char32_t unicode) const;

	void addAction(const QString& name, QAction* action);
	void setShortcut(char32_t unicode, const QKeySequence& sequence);
	void setShortcuts(const QHash<QString, QKeySequence>& shortcuts);

	static ActionManager* instance()
	{
		return m_instance;
	}

Q_SIGNALS:
	void insertText(const QString& text);

private Q_SLOTS:
	void symbolShortcutActivated();

private:
	void addShortcut(char32_t unicode, const QKeySequence& sequence);

private:
	QWidget* m_widget;
	QHash<QString, Action> m_actions;
	QHash<char32_t, QShortcut*> m_symbol_shortcuts;
	QHash<const QObject*, QString> m_symbol_shortcuts_text;
	static ActionManager* m_instance;
};

#endif // FOCUSWRITER_ACTION_MANAGER_H
