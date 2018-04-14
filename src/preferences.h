/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2017 Graeme Gott <graeme@gottcode.org>
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

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

	RangedInt goalType() const;
	RangedInt goalMinutes() const;
	RangedInt goalWords() const;
	bool goalHistory() const;
	bool goalStreaks() const;
	RangedInt goalStreakMinimum() const;
	void setGoalType(int goal);
	void setGoalMinutes(int goal);
	void setGoalWords(int goal);
	void setGoalHistory(bool enable);
	void setGoalStreaks(bool enable);
	void setGoalStreakMinimum(int percent);

	bool showCharacters() const;
	bool showPages() const;
	bool showParagraphs() const;
	bool showWords() const;
	void setShowCharacters(bool show);
	void setShowPages(bool show);
	void setShowParagraphs(bool show);
	void setShowWords(bool show);

	RangedInt pageType() const;
	RangedInt pageCharacters() const;
	RangedInt pageParagraphs() const;
	RangedInt pageWords() const;
	void setPageType(int type);
	void setPageCharacters(int characters);
	void setPageParagraphs(int paragraphs);
	void setPageWords(int words);

	RangedInt wordcountType() const;
	void setWordcountType(int type);

	bool alwaysCenter() const;
	bool blockCursor() const;
	bool smoothFonts() const;
	bool smartQuotes() const;
	int doubleQuotes() const;
	int singleQuotes() const;
	bool typewriterSounds() const;
	void setAlwaysCenter(bool center);
	void setBlockCursor(bool block);
	void setSmoothFonts(bool smooth);
	void setSmartQuotes(bool quotes);
	void setDoubleQuotes(int quotes);
	void setSingleQuotes(int quotes);
	void setTypewriterSounds(bool sounds);

	QString sceneDivider() const;
	void setSceneDivider(const QString& divider);

	bool savePositions() const;
	bool writeByteOrderMark() const;
	RangedString saveFormat() const;
	void setSavePositions(bool save);
	void setWriteByteOrderMark(bool write_bom);
	void setSaveFormat(const QString& format);

	bool alwaysShowScrollBar() const;
	void setAlwaysShowScrollbar(bool show_scrollbar);
	bool alwaysShowHeader() const;
	void setAlwaysShowHeader(bool show_header);
	bool alwaysShowFooter() const;
	void setAlwaysShowFooter(bool show_footer);

	int toolbarStyle() const;
	QStringList toolbarActions() const;
	void setToolbarStyle(int style);
	void setToolbarActions(const QStringList& actions);

	bool highlightMisspelled() const;
	bool ignoredWordsWithNumbers() const;
	bool ignoredUppercaseWords() const;
	QString language() const;
	void setHighlightMisspelled(bool highlight);
	void setIgnoreWordsWithNumbers(bool ignore);
	void setIgnoreUppercaseWords(bool ignore);
	void setLanguage(const QString& language);

private:
	Preferences();

	void reload();
	void write();

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

#endif
