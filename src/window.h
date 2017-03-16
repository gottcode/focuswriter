/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2017 Graeme Gott <graeme@gottcode.org>
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

class DailyProgress;
class DailyProgressDialog;
class DailyProgressLabel;
class Document;
class DocumentCache;
class DocumentWatcher;
class LoadScreen;
class SessionManager;
class Sound;
class Stack;
class TimerManager;

#include <QHash>
#include <QMainWindow>
class QAction;
class QActionGroup;
class QLabel;
class QSettings;
class QTabBar;
class QToolBar;

class Window : public QMainWindow
{
	Q_OBJECT

public:
	Window(const QStringList& command_line_files);

	void addDocuments(const QStringList& files, const QStringList& datafiles, const QStringList& positions = QStringList(), int active = -1, bool show_load = false);
	void addDocuments(QDropEvent* event);
	bool closeDocuments(QSettings* session);
	bool saveDocuments(QSettings* session);

public slots:
	void addDocuments(const QString& documents);

protected:
	virtual void changeEvent(QEvent* event);
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dropEvent(QDropEvent* event);
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
	void closeDocument(Document* document);
	void showDocument(Document* document);
	void nextDocument();
	void previousDocument();
	void firstDocument();
	void lastDocument();
	void setLanguageClicked();
	void minimize();
	void toggleFullscreen();
	void toggleToolbar(bool visible);
	void toggleMenuIcons(bool visible);
	void themeClicked();
	void preferencesClicked();
	void aboutClicked();
	void setLocaleClicked();
	void tabClicked(int index);
	void tabMoved(int from, int to);
	void tabClosed(int index);
	void updateClock();
	void updateDetails();
	void updateFormatActions();
	void updateFormatAlignmentActions();
	void updateSave();

private:
	bool addDocument(const QString& file = QString(), const QString& datafile = QString(), int position = -1);
	void closeDocument(int index, bool allow_empty = false);
	void queueDocuments(const QStringList& files);
	bool saveDocument(int index);
	void loadPreferences();
	void hideInterface();
	void updateMargin();
	void updateTab(int index);
	void updateWriteState(int index);
	void initMenus();

private:
	QToolBar* m_toolbar;
	QHash<QString, QAction*> m_actions;
	QList<QAction*> m_format_actions;
	QAction* m_replace_document_quotes;
	QAction* m_replace_selection_quotes;
	QActionGroup* m_focus_actions;
	QActionGroup* m_headings_actions;

	Stack* m_documents;
	DocumentCache* m_document_cache;
	DocumentWatcher* m_document_watcher;
	QThread* m_document_cache_thread;
	QStringList m_queued_documents;

	LoadScreen* m_load_screen;
	bool m_loading;

	QTabBar* m_tabs;
	SessionManager* m_sessions;
	TimerManager* m_timers;
	Sound* m_key_sound;
	Sound* m_enter_key_sound;

	QWidget* m_footer;
	QLabel* m_character_label;
	QLabel* m_page_label;
	QLabel* m_paragraph_label;
	QLabel* m_wordcount_label;
	DailyProgressLabel* m_progress_label;
	QLabel* m_clock_label;
	QTimer* m_clock_timer;
	QTimer* m_save_timer;

	bool m_fullscreen;
	bool m_save_positions;
	DailyProgress* m_daily_progress;
	DailyProgressDialog* m_daily_progress_dialog;
};

#endif
