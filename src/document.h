/*
	SPDX-FileCopyrightText: 2009-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DOCUMENT_H
#define FOCUSWRITER_DOCUMENT_H

#include "dictionary_ref.h"
#include "document_writer.h"
#include "stats.h"
class Alert;
class DailyProgress;
class Highlighter;
class SceneList;
class SceneModel;
class Theme;

#include <QHash>
#include <QSharedPointer>
#include <QTextBlockFormat>
#include <QTime>
#include <QWidget>
class QGridLayout;
class QScrollBar;
class QPrinter;
class QTextEdit;
class QTimer;

class Document : public QWidget
{
	Q_OBJECT

public:
	Document(const QString& filename, DailyProgress* daily_progress, QWidget* parent = nullptr);
	~Document();

	QString filename() const;
	QString title() const;
	int untitledIndex() const;
	bool isModified() const;
	bool isReadOnly() const;
	bool isRichText() const;
	int characterCount() const;
	int characterAndSpaceCount() const;
	int pageCount() const;
	int paragraphCount() const;
	int wordCount() const;
	int wordCountDelta() const;
	SceneModel* sceneModel() const;
	QTextEdit* text() const;

	void cache();
	bool save();
	bool saveAs();
	bool rename();
	void reload(bool prompt = true);
	void close();
	void checkSpelling();
	void print(QPrinter* printer);
	bool loadFile(const QString& filename, int position);
	void loadTheme(const Theme& theme);
	void loadPreferences();
	void setFocusMode(int focus_mode);
	void setModified(bool modified);
	void setRichText(bool rich_text);
	void setScrollBarVisible(bool visible);
	void setSceneList(SceneList* scene_list);

	bool eventFilter(QObject* watched, QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;

public Q_SLOTS:
	void centerCursor(bool force = false);

Q_SIGNALS:
	void alert(Alert* alert);
	void replaceCacheFile(const Document* document, const QString& file);
	void writeCacheFile(const Document* document, QSharedPointer<DocumentWriter> writer);
	void changed();
	void changedName();
	void loadStarted(const QString& path);
	void loadFinished();
	void footerVisible(bool visible);
	void headerVisible(bool visible);
	void scenesVisible(bool visible);
	void indentChanged(bool indented);
	void modificationChanged(bool modified);
	void alignmentChanged();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private Q_SLOTS:
	void cursorPositionChanged();
	void focusText();
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
	QString getSaveFileName(const QString& title);
	bool processFileName(const QString& filename);
	void updateSaveLocation();
	void updateSaveName();
	void updateState();

private:
	QString m_filename;
	QString m_default_format;
	bool m_cache_outdated;
	QHash<int, QPair<QString, bool>> m_old_states;
	int m_index;
	bool m_always_center;
	bool m_mouse_button_down;
	bool m_block_cursor;
	bool m_rich_text;
	bool m_spacings_loaded;
	int m_focus_mode;
	QTextBlockFormat m_block_format;

	QTimer* m_hide_timer;

	QGridLayout* m_layout;
	QTextEdit* m_text;
	QScrollBar* m_scrollbar;
	SceneList* m_scene_list;
	SceneModel* m_scene_model;
	DictionaryRef m_dictionary;
	Highlighter* m_highlighter;
	QColor m_text_color;

	Stats* m_stats;
	Stats m_document_stats;
	Stats m_selected_stats;
	Stats m_cached_stats;
	int m_cached_block_count;
	int m_cached_current_block;
	int m_saved_wordcount;

	int m_page_type;
	float m_page_amount;

	int m_wordcount_type;

	DailyProgress* m_daily_progress;
};

inline QString Document::filename() const
{
	return m_filename;
}

inline int Document::untitledIndex() const
{
	return m_index;
}

inline bool Document::isRichText() const
{
	return m_rich_text;
}

inline int Document::characterCount() const
{
	return m_stats->characterCount();
}

inline int Document::characterAndSpaceCount() const
{
	return m_stats->characterAndSpaceCount();
}

inline int Document::pageCount() const
{
	return m_stats->pageCount();
}

inline int Document::paragraphCount() const
{
	return m_stats->paragraphCount();
}

inline int Document::wordCount() const
{
	return m_stats->wordCount();
}

inline int Document::wordCountDelta() const
{
	return m_document_stats.wordCount() - m_saved_wordcount;
}

inline SceneModel* Document::sceneModel() const
{
	return m_scene_model;
}

inline QTextEdit* Document::text() const
{
	return m_text;
}

#endif // FOCUSWRITER_DOCUMENT_H
