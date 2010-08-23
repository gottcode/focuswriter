/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef WINDOW_H
#define WINDOW_H

#include <QHash>
#include <QMainWindow>
class QAction;
class QLabel;
class QSettings;
class QTabBar;
class QToolBar;
class LoadScreen;
class Preferences;
class SessionManager;
class Stack;
class TimerManager;

class Window : public QMainWindow {
	Q_OBJECT
public:
	Window();

	void addDocuments(const QStringList& files, const QStringList& positions = QStringList(), int active = 0, bool show_load = false);
	bool closeDocuments(QSettings* session = 0);

protected:
	virtual bool event(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	virtual void leaveEvent(QEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private slots:
	void newDocument();
	void openDocument();
	void renameDocument();
	void saveAllDocuments();
	void closeDocument();
	void nextDocument();
	void previousDocument();
	void toggleFullscreen();
	void toggleToolbar(bool visible);
	void toggleMenuIcons(bool visible);
	void themeClicked();
	void preferencesClicked();
	void aboutClicked();
	void tabClicked(int index);
	void tabMoved(int from, int to);
	void tabClosed(int index);
	void updateClock();
	void updateDetails();
	void updateFormatActions();
	void updateFormatAlignmentActions();
	void updateProgress();
	void updateSave();

private:
	void addDocument(const QString& filename = QString(), int position = -1);
	bool saveDocument(int index);
	void loadPreferences(const Preferences& preferences);
	void hideInterface();
	void updateMargin();
	void updateTab(int index);
	void initMenus();

private:
	QToolBar* m_toolbar;
	QHash<QString, QAction*> m_actions;
	QList<QAction*> m_format_actions;
	QAction* m_plaintext_action;
	QAction* m_richtext_action;
	QString m_open_filter;

	LoadScreen* m_load_screen;
	Stack* m_documents;
	QTabBar* m_tabs;
	SessionManager* m_sessions;
	TimerManager* m_timers;

	QWidget* m_footer;
	QLabel* m_character_label;
	QLabel* m_page_label;
	QLabel* m_paragraph_label;
	QLabel* m_wordcount_label;
	QLabel* m_progress_label;
	QLabel* m_clock_label;
	QTimer* m_clock_timer;

	bool m_fullscreen;
	bool m_auto_save;
	bool m_save_positions;
	int m_goal_type;
	int m_time_goal;
	int m_wordcount_goal;
	int m_current_time;
	int m_current_wordcount;
};

#endif
