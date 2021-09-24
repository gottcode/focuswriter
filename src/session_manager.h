/*
	SPDX-FileCopyrightText: 2010-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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
