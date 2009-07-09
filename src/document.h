/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
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

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QFuture>
#include <QTime>
#include <QWidget>
class QGridLayout;
class QScrollBar;
class QPlainTextEdit;
class QTimer;
class FindDialog;
class Highlighter;
class Preferences;
class Theme;

class Document : public QWidget {
	Q_OBJECT
public:
	Document(const QString& filename, int& current_wordcount, int& current_time, QWidget* parent = 0);
	~Document();

	QString filename() const {
		return m_filename;
	}

	int index() const {
		return m_index;
	}

	int characterCount() const {
		return m_character_count - m_space_count;
	}

	int characterAndSpaceCount() const {
		return m_character_count;
	}

	int pageCount() const {
		return m_page_count;
	}

	int paragraphCount() const {
		return m_paragraph_count;
	}

	int wordCount() const {
		return m_wordcount;
	}

	QPlainTextEdit* text() const {
		return m_text;
	}

	bool save();
	bool saveAs();
	bool rename();
	void checkSpelling();
	void find();
	void print();
	void loadTheme(const Theme& theme);
	void loadPreferences(const Preferences& preferences);
	void setMargin(int margin);

	virtual bool eventFilter(QObject* watched, QEvent* event);

signals:
	void changed();
	void footerVisible(bool visible);
	void headerVisible(bool visible);

protected:
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private slots:
	void hideMouse();
	void updateWordCount(int position, int removed, int added);

private:
	void calculateWordCount();
	void updateSaveLocation();

private:
	QString m_filename;
	int m_index;
	bool m_auto_append;
	bool m_block_cursor;
	QFuture<void> m_file_save;

	FindDialog* m_find_dialog;
	QTimer* m_hide_timer;

	QGridLayout* m_layout;
	QPlainTextEdit* m_text;
	QScrollBar* m_scrollbar;
	Highlighter* m_highlighter;
	int m_margin;

	int m_character_count;
	int m_page_count;
	int m_paragraph_count;
	int m_space_count;
	int m_wordcount;

	// Daily progress
	int& m_current_wordcount;
	int m_wordcount_goal;
	QTime m_time;
	int& m_current_time;
	int m_time_goal;
};

#endif
