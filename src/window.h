/***********************************************************************
 *
 * Copyright (C) 2008-2009 Graeme Gott <graeme@gottcode.org>
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

#include <QLabel>
#include <QTime>
class QAction;
class QGridLayout;
class QScrollBar;
class QPlainTextEdit;
class QTextBlock;
class QTimer;
class QToolBar;
class FindDialog;
class Preferences;
class Theme;

class Window : public QLabel {
	Q_OBJECT
public:
	Window(int& current_wordcount, int& current_time);

	void open(const QString& filename);

	virtual bool eventFilter(QObject* watched, QEvent* event);

protected:
	virtual bool event(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private slots:
	void newClicked();
	void openClicked();
	void saveClicked();
	void renameClicked();
	void printClicked();
	void themeClicked();
	void preferencesClicked();
	void aboutClicked();
	void setFullscreen(bool fullscreen);
	void hideMouse();
	void themeSelected(const Theme& theme);
	void updateWordCount(int position, int removed, int added);
	void updateClock();

private:
	int calculateWordCount();
	void loadTheme(const Theme& theme);
	void loadPreferences(const Preferences& preferences);
	void updateBackground();
	void updateProgress();

private:
	QString m_filename;
	QImage m_background;
	QString m_background_path;
	QString m_background_cached;
	int m_background_position;
	QToolBar* m_toolbar;
	QAction* m_fullscreen_action;
	FindDialog* m_find_dialog;
	QPlainTextEdit* m_text;
	QScrollBar* m_scrollbar;
	QWidget* m_details;
	QLabel* m_filename_label;
	QLabel* m_wordcount_label;
	QLabel* m_progress_label;
	QLabel* m_clock_label;
	int m_margin;
	QTimer* m_clock_timer;
	QTimer* m_hide_timer;
	bool m_auto_save;
	bool m_auto_append;
	bool m_loaded;

	// Daily progress
	int m_wordcount;
	int& m_current_wordcount;
	int m_wordcount_goal;
	QTime m_time;
	int& m_current_time;
	int m_time_goal;
	int m_goal_type;
};

#endif
