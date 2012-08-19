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

#ifndef ACTION_MANAGER_H
#define ACTION_MANAGER_H

#include <QHash>
#include <QKeySequence>
#include <QObject>
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
	ActionManager(QWidget* parent = 0);
	~ActionManager();

	QList<QString> actions() const
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
	QKeySequence shortcut(quint32 unicode);

	void addAction(const QString& name, QAction* action);
	void setShortcut(quint32 unicode, const QKeySequence& sequence);
	void setShortcuts(const QHash<QString, QKeySequence>& shortcuts);

	static ActionManager* instance()
	{
		return m_instance;
	}

signals:
	void insertText(const QString& text);

private slots:
	void symbolShortcutActivated();

private:
	void addShortcut(quint32 unicode, const QKeySequence& sequence);

private:
	QWidget* m_widget;
	QHash<QString, Action> m_actions;
	QHash<quint32, QShortcut*> m_symbol_shortcuts;
	QHash<QObject*, QString> m_symbol_shortcuts_text;
	static ActionManager* m_instance;
};

#endif
