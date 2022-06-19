/*
	SPDX-FileCopyrightText: 2008-2017 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_WINDOW_H
#define FOCUSWRITER_WINDOW_H

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
	explicit Window(const QStringList& command_line_files);

	void addDocuments(const QStringList& files, const QStringList& datafiles, const QStringList& positions = QStringList(), int active = -1, bool show_load = false);
	void addDocuments(QDropEvent* event);
	bool closeDocuments(QSettings* session);
	bool saveDocuments(QSettings* session);

public Q_SLOTS:
	void addDocuments(const QString& documents);

protected:
	void changeEvent(QEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	bool event(QEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
	void newDocument();
	void openDocument();
	void renameDocument();
	void saveAllDocuments();
	void closeDocument();
	void closeDocument(const Document* document);
	void showDocument(const Document* document);
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

#endif // FOCUSWRITER_WINDOW_H
