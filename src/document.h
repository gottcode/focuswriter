/***********************************************************************
 *
 * Copyright (C) 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "stats.h"
class Highlighter;
class Preferences;
class Theme;

#include <QHash>
#include <QTime>
#include <QWidget>
class QGridLayout;
class QScrollBar;
class QTextEdit;
class QTimer;

class Document : public QWidget {
	Q_OBJECT
public:
	Document(const QString& filename, int& current_wordcount, int& current_time, const QString& theme, QWidget* parent = 0);
	~Document();

	QString filename() const {
		return m_filename;
	}

	bool isRichText() const {
		return m_rich_text;
	}

	int index() const {
		return m_index;
	}

	int characterCount() const {
		return m_document_stats.characterCount();
	}

	int characterAndSpaceCount() const {
		return m_document_stats.characterAndSpaceCount();
	}

	int pageCount() const {
		return m_document_stats.pageCount();
	}

	int paragraphCount() const {
		return m_document_stats.paragraphCount();
	}

	int wordCount() const {
		return m_document_stats.wordCount();
	}

	QTextEdit* text() const {
		return m_text;
	}

	bool save();
	bool saveAs();
	bool rename();
	void checkSpelling();
	void print();
	void loadTheme(const Theme& theme);
	void loadPreferences(const Preferences& preferences);
	void setRichText(bool rich_text);
	void setScrollBarVisible(bool visible);

	virtual bool eventFilter(QObject* watched, QEvent* event);

public slots:
	void centerCursor(bool force = false);

signals:
	void changed();
	void changedName();
	void footerVisible(bool visible);
	void headerVisible(bool visible);
	void indentChanged(bool indented);
	void alignmentChanged();

protected:
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private slots:
	void cursorPositionChanged();
	void hideMouse();
	void scrollBarActionTriggered(int action);
	void scrollBarRangeChanged(int min, int max);
	void undoCommandAdded();
	void updateWordCount(int position, int removed, int added);

private:
	void calculateWordCount();
	void clearIndex();
	void findIndex();
	QString fileFilter(const QString& filename) const;
	QString fileNameWithExtension(const QString& filename, const QString& filter) const;
	void updateSaveLocation();

private:
	QString m_filename;
	QHash<int, QString> m_old_filenames;
	int m_index;
	bool m_always_center;
	bool m_block_cursor;
	bool m_rich_text;

	QTimer* m_hide_timer;

	QGridLayout* m_layout;
	QTextEdit* m_text;
	QScrollBar* m_scrollbar;
	Highlighter* m_highlighter;

	Stats m_document_stats;
	Stats m_cached_stats;
	int m_cached_block_count;
	int m_cached_current_block;

	int m_page_type;
	float m_page_amount;

	bool m_accurate_wordcount;

	// Daily progress
	int& m_current_wordcount;
	int m_wordcount_goal;
	QTime m_time;
	int& m_current_time;
	int m_time_goal;
};

#endif
