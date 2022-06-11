/*
	SPDX-FileCopyrightText: 2010-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SESSION_MANAGER_H
#define FOCUSWRITER_SESSION_MANAGER_H

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
	explicit SessionManager(Window* parent);
	~SessionManager();

	Session* current() const;
	QMenu* menu() const;

	bool closeCurrent();
	bool saveCurrent();
	void setCurrent(const QString& id, const QStringList& files = QStringList(), const QStringList& datafiles = QStringList());

public Q_SLOTS:
	void newSession();

Q_SIGNALS:
	void themeChanged(const Theme& theme);

protected:
	void hideEvent(QHideEvent* event) override;

private Q_SLOTS:
	void renameSession();
	void cloneSession();
	void deleteSession();
	void switchSession();
	void switchSession(const QAction* session);
	void selectedSessionChanged(const QListWidgetItem* session);

private:
	QString getSessionName(const QString& title, const QString& session = QString());
	const QListWidgetItem* selectedSession(bool prevent_default) const;
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

#endif // FOCUSWRITER_SESSION_MANAGER_H
