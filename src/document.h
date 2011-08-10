/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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
class Dictionary;
class Highlighter;
class FocusMode;
class Preferences;
class Theme;

#include <QHash>
#include <QTime>
#include <QWidget>
class QGridLayout;
class QScrollBar;
class QTextEdit;
class QTimer;

class Document : public QWidget
{
	Q_OBJECT

public:
	Document(const QString& filename, int& current_wordcount, int& current_time, QWidget* parent = 0);
	~Document();

	QString cacheFilename() const;
	QString filename() const;
	int untitledIndex() const;
	bool isReadOnly() const;
	bool isRichText() const;
	int characterCount() const;
	int characterAndSpaceCount() const;
	int pageCount() const;
	int paragraphCount() const;
	int wordCount() const;
	QTextEdit* text() const;
	FocusMode* fMode() const;

	void cache();
	bool save();
	bool saveAs();
	bool rename();
	void checkSpelling();
	void print();
	void loadFile(const QString& filename, int position);
	void loadTheme(const Theme& theme);
	void loadPreferences(const Preferences& preferences);
	void setRichText(bool rich_text);
	void setScrollBarVisible(bool visible);

	virtual bool eventFilter(QObject* watched, QEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);

	static QString cachePath();
	static void setCachePath(const QString& path);

public slots:
	void centerCursor(bool force = false);

signals:
	void changed();
	void changedName();
	void footerVisible(bool visible);
	void headerVisible(bool visible);
	void formattingEnabled(bool enabled);
	void indentChanged(bool indented);
	void alignmentChanged();

protected:
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private slots:
	void cursorPositionChanged();
	void hideMouse();
	void scrollBarActionTriggered(int action);
	void scrollBarRangeChanged(int min, int max);
	void dictionaryChanged();
	void selectionChanged();
	void undoCommandAdded();
	void updateWordCount(int position, int removed, int added);

private:
	void calculateWordCount();
	void clearIndex();
	void findIndex();
	QString fileFilter(const QString& filename) const;
	QString fileNameWithExtension(const QString& filename, const QString& filter) const;
	void updateSaveLocation();
	void updateState();
	bool writeFile(const QString& filename);

private:
	QString m_filename;
	QString m_cache_filename;
	QByteArray m_codepage;
	QHash<int, QPair<QString, bool> > m_old_states;
	int m_index;
	bool m_always_center;
	bool m_block_cursor;
	bool m_rich_text;

	QTimer* m_hide_timer;

	QGridLayout* m_layout;
	QTextEdit* m_text;
	QScrollBar* m_scrollbar;
	Dictionary* m_dictionary;
	Highlighter* m_highlighter;
	FocusMode* m_focusmode;

	Stats* m_stats;
	Stats m_document_stats;
	Stats m_selected_stats;
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

inline QString Document::cacheFilename() const {
	return m_cache_filename;
}

inline QString Document::filename() const {
	return m_filename;
}

inline int Document::untitledIndex() const {
	return m_index;
}

inline bool Document::isRichText() const {
	return m_rich_text;
}

inline int Document::characterCount() const {
	return m_stats->characterCount();
}

inline int Document::characterAndSpaceCount() const {
	return m_stats->characterAndSpaceCount();
}

inline int Document::pageCount() const {
	return m_stats->pageCount();
}

inline int Document::paragraphCount() const {
	return m_stats->paragraphCount();
}

inline int Document::wordCount() const {
	return m_stats->wordCount();
}

inline QTextEdit* Document::text() const {
	return m_text;
}

inline FocusMode* Document::fMode() const {
    return m_focusmode;
}

#endif
