/***********************************************************************
 *
 * Copyright (C) 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

class Session;
class Theme;
class Window;

#include <QDialog>
class QActionGroup;
class QListWidget;
class QListWidgetItem;
class QMenu;

class SessionManager : public QDialog
{
	Q_OBJECT

public:
	SessionManager(Window* parent);
	~SessionManager();

	Session* current() const;
	QMenu* menu() const;

	bool closeCurrent();
	bool saveCurrent();
	void setCurrent(const QString& id, const QStringList& files = QStringList(), const QStringList& datafiles = QStringList());

public slots:
	void newSession();

signals:
	void themeChanged(const Theme& theme);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void renameSession();
	void cloneSession();
	void deleteSession();
	void switchSession();
	void switchSession(QAction* session);
	void selectedSessionChanged(QListWidgetItem* session);

private:
	QString getSessionName(const QString& title, const QString& session = QString());
	QListWidgetItem* selectedSession(bool prevent_default);
	void updateList(const QString& selected);

private:
	Session* m_session;
	Window* m_window;
	QMenu* m_sessions_menu;
	QActionGroup* m_sessions_actions;
	QListWidget* m_sessions_list;
	QPushButton* m_rename_button;
	QPushButton* m_delete_button;
	QPushButton* m_switch_button;
};

#endif
