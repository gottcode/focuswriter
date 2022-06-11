/*
	SPDX-FileCopyrightText: 2008-2017 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_PREFERENCES_H
#define FOCUSWRITER_PREFERENCES_H

#include "ranged_int.h"
#include "ranged_string.h"
#include "settings_file.h"

#include <QStringList>

class Preferences : public SettingsFile
{
public:
	~Preferences();

	static Preferences& instance()
	{
		static Preferences preferences;
		return preferences;
	}

	RangedInt goalType() const { return m_goal_type; }
	RangedInt goalMinutes() const { return m_goal_minutes; }
	RangedInt goalWords() const { return m_goal_words; }
	bool goalHistory() const { return m_goal_history; }
	bool goalStreaks() const { return m_goal_streaks; }
	RangedInt goalStreakMinimum() const { return m_goal_streak_minimum; }
	void setGoalType(int goal) { setValue(m_goal_type, goal); }
	void setGoalMinutes(int goal) { setValue(m_goal_minutes, goal); }
	void setGoalWords(int goal) { setValue(m_goal_words, goal); }
	void setGoalHistory(bool enable) { setValue(m_goal_history, enable); }
	void setGoalStreaks(bool enable) { setValue(m_goal_streaks, enable); }
	void setGoalStreakMinimum(int percent) { setValue(m_goal_streak_minimum, percent); }

	bool showCharacters() const { return m_show_characters; }
	bool showPages() const { return m_show_pages; }
	bool showParagraphs() const { return m_show_paragraphs; }
	bool showWords() const { return m_show_words; }
	void setShowCharacters(bool show) { setValue(m_show_characters, show); }
	void setShowPages(bool show) { setValue(m_show_pages, show); }
	void setShowParagraphs(bool show) { setValue(m_show_paragraphs, show); }
	void setShowWords(bool show) { setValue(m_show_words, show); }

	RangedInt pageType() const { return m_page_type; }
	RangedInt pageCharacters() const { return m_page_characters; }
	RangedInt pageParagraphs() const { return m_page_paragraphs; }
	RangedInt pageWords() const { return m_page_words; }
	void setPageType(int type) { setValue(m_page_type, type); }
	void setPageCharacters(int characters) { setValue(m_page_characters, characters); }
	void setPageParagraphs(int paragraphs) { setValue(m_page_paragraphs, paragraphs); }
	void setPageWords(int words) { setValue(m_page_words, words); }

	RangedInt wordcountType() const { return m_wordcount_type; }
	void setWordcountType(int type) { setValue(m_wordcount_type, type); }

	bool alwaysCenter() const { return m_always_center; }
	bool blockCursor() const { return m_block_cursor; }
	bool smoothFonts() const { return m_smooth_fonts; }
	bool smartQuotes() const { return m_smart_quotes; }
	int doubleQuotes() const { return m_double_quotes; }
	int singleQuotes() const { return m_single_quotes; }
	bool typewriterSounds() const { return m_typewriter_sounds; }
	void setAlwaysCenter(bool center) { setValue(m_always_center, center); }
	void setBlockCursor(bool block) { setValue(m_block_cursor, block); }
	void setSmoothFonts(bool smooth) { setValue(m_smooth_fonts, smooth); }
	void setSmartQuotes(bool quotes) { setValue(m_smart_quotes, quotes); }
	void setDoubleQuotes(int quotes) { setValue(m_double_quotes, quotes); }
	void setSingleQuotes(int quotes) { setValue(m_single_quotes, quotes); }
	void setTypewriterSounds(bool sounds){ setValue(m_typewriter_sounds, sounds); }

	QString sceneDivider() const{ return m_scene_divider; }
	void setSceneDivider(const QString& divider);

	bool savePositions() const { return m_save_positions; }
	bool writeByteOrderMark() const { return m_write_bom; }
	RangedString saveFormat() const { return m_save_format; }
	void setSavePositions(bool save) { setValue(m_save_positions, save); }
	void setWriteByteOrderMark(bool write_bom) { setValue(m_write_bom, write_bom); }
	void setSaveFormat(const QString& format) { setValue(m_save_format, format); }

	bool alwaysShowScrollBar() const { return m_always_show_scrollbar; }
	bool alwaysShowHeader() const { return m_always_show_header; }
	bool alwaysShowFooter() const { return m_always_show_footer; }
	void setAlwaysShowScrollbar(bool show_scrollbar) { setValue(m_always_show_scrollbar, show_scrollbar); }
	void setAlwaysShowHeader(bool show_header) { setValue(m_always_show_header, show_header); }
	void setAlwaysShowFooter(bool show_footer) { setValue(m_always_show_footer, show_footer); }

	int toolbarStyle() const { return m_toolbar_style; }
	QStringList toolbarActions() const { return m_toolbar_actions; }
	void setToolbarStyle(int style) { setValue(m_toolbar_style, style); }
	void setToolbarActions(const QStringList& actions) { setValue(m_toolbar_actions, actions); }

	bool highlightMisspelled() const { return m_highlight_misspelled; }
	bool ignoredWordsWithNumbers() const { return m_ignore_numbers; }
	bool ignoredUppercaseWords() const { return m_ignore_uppercase; }
	QString language() const { return m_language; }
	void setHighlightMisspelled(bool highlight) { setValue(m_highlight_misspelled, highlight); }
	void setIgnoreWordsWithNumbers(bool ignore) { setValue(m_ignore_numbers, ignore); }
	void setIgnoreUppercaseWords(bool ignore) { setValue(m_ignore_uppercase, ignore); }
	void setLanguage(const QString& language);

private:
	explicit Preferences();

	void reload() override;
	void write() override;

private:
	RangedInt m_goal_type;
	RangedInt m_goal_minutes;
	RangedInt m_goal_words;
	bool m_goal_history;
	bool m_goal_streaks;
	RangedInt m_goal_streak_minimum;

	bool m_show_characters;
	bool m_show_pages;
	bool m_show_paragraphs;
	bool m_show_words;

	RangedInt m_page_type;
	RangedInt m_page_characters;
	RangedInt m_page_paragraphs;
	RangedInt m_page_words;

	RangedInt m_wordcount_type;

	bool m_always_center;
	bool m_block_cursor;
	bool m_smooth_fonts;
	bool m_smart_quotes;
	int m_double_quotes;
	int m_single_quotes;
	bool m_typewriter_sounds;

	QString m_scene_divider;

	bool m_save_positions;
	RangedString m_save_format;
	bool m_write_bom;

	int m_toolbar_style;
	QStringList m_toolbar_actions;

	bool m_highlight_misspelled;
	bool m_ignore_uppercase;
	bool m_ignore_numbers;
	QString m_language;

	bool m_always_show_scrollbar;
	bool m_always_show_header;
	bool m_always_show_footer;
};

#endif // FOCUSWRITER_PREFERENCES_H
