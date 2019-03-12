/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2017 Graeme Gott <graeme@gottcode.org>
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

#include "preferences.h"

#include "dictionary_manager.h"
#include "format_manager.h"
#include "scene_model.h"

#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QStyle>

//-----------------------------------------------------------------------------

Preferences::Preferences() :
	m_goal_type(0, 2),
	m_goal_minutes(5, 1440),
	m_goal_words(100, 100000),
	m_goal_streak_minimum(1, 100),
	m_page_type(0, 2),
	m_page_characters(1, 10000),
	m_page_paragraphs(1, 100),
	m_page_words(1, 2000),
	m_wordcount_type(0, 2),
	m_save_format(FormatManager::types())
{
	forgetChanges();
}

//-----------------------------------------------------------------------------

Preferences::~Preferences()
{
	saveChanges();
}

//-----------------------------------------------------------------------------

RangedInt Preferences::goalType() const
{
	return m_goal_type;
}

//-----------------------------------------------------------------------------

RangedInt Preferences::goalMinutes() const
{
	return m_goal_minutes;
}

//-----------------------------------------------------------------------------

RangedInt Preferences::goalWords() const
{
	return m_goal_words;
}

//-----------------------------------------------------------------------------

bool Preferences::goalHistory() const
{
	return m_goal_history;
}

//-----------------------------------------------------------------------------

bool Preferences::goalStreaks() const
{
	return m_goal_streaks;
}

//-----------------------------------------------------------------------------

RangedInt Preferences::goalStreakMinimum() const
{
	return m_goal_streak_minimum;
}

//-----------------------------------------------------------------------------

void Preferences::setGoalType(int goal)
{
	setValue(m_goal_type, goal);
}

//-----------------------------------------------------------------------------

void Preferences::setGoalMinutes(int goal)
{
	setValue(m_goal_minutes, goal);
}

//-----------------------------------------------------------------------------

void Preferences::setGoalWords(int goal)
{
	setValue(m_goal_words, goal);
}

//-----------------------------------------------------------------------------

void Preferences::setGoalHistory(bool enable)
{
	setValue(m_goal_history, enable);
}

//-----------------------------------------------------------------------------

void Preferences::setGoalStreaks(bool enable)
{
	setValue(m_goal_streaks, enable);
}

//-----------------------------------------------------------------------------

void Preferences::setGoalStreakMinimum(int percent)
{
	setValue(m_goal_streak_minimum, percent);
}

//-----------------------------------------------------------------------------

bool Preferences::showCharacters() const
{
	return m_show_characters;
}

//-----------------------------------------------------------------------------

bool Preferences::showPages() const
{
	return m_show_pages;
}

//-----------------------------------------------------------------------------

bool Preferences::showParagraphs() const
{
	return m_show_paragraphs;
}

//-----------------------------------------------------------------------------

bool Preferences::showWords() const
{
	return m_show_words;
}

//-----------------------------------------------------------------------------

void Preferences::setShowCharacters(bool show)
{
	setValue(m_show_characters, show);
}

//-----------------------------------------------------------------------------

void Preferences::setShowPages(bool show)
{
	setValue(m_show_pages, show);
}

//-----------------------------------------------------------------------------

void Preferences::setShowParagraphs(bool show)
{
	setValue(m_show_paragraphs, show);
}

//-----------------------------------------------------------------------------

void Preferences::setShowWords(bool show)
{
	setValue(m_show_words, show);
}

//-----------------------------------------------------------------------------

RangedInt Preferences::pageType() const
{
	return m_page_type;
}

//-----------------------------------------------------------------------------

RangedInt Preferences::pageCharacters() const
{
	return m_page_characters;
}

//-----------------------------------------------------------------------------

RangedInt Preferences::pageParagraphs() const
{
	return m_page_paragraphs;
}

//-----------------------------------------------------------------------------

RangedInt Preferences::pageWords() const
{
	return m_page_words;
}

//-----------------------------------------------------------------------------

void Preferences::setPageType(int type)
{
	setValue(m_page_type, type);
}

//-----------------------------------------------------------------------------

void Preferences::setPageCharacters(int characters)
{
	setValue(m_page_characters, characters);
}

//-----------------------------------------------------------------------------

void Preferences::setPageParagraphs(int paragraphs)
{
	setValue(m_page_paragraphs, paragraphs);
}

//-----------------------------------------------------------------------------

void Preferences::setPageWords(int words)
{
	setValue(m_page_words, words);
}

//-----------------------------------------------------------------------------

RangedInt Preferences::wordcountType() const
{
	return m_wordcount_type;
}

//-----------------------------------------------------------------------------

void Preferences::setWordcountType(int type)
{
	setValue(m_wordcount_type, type);
}

//-----------------------------------------------------------------------------

bool Preferences::alwaysCenter() const
{
	return m_always_center;
}

//-----------------------------------------------------------------------------

bool Preferences::blockCursor() const
{
	return m_block_cursor;
}

//-----------------------------------------------------------------------------

bool Preferences::smoothFonts() const
{
	return m_smooth_fonts;
}

//-----------------------------------------------------------------------------

bool Preferences::smartQuotes() const
{
	return m_smart_quotes;
}

//-----------------------------------------------------------------------------

int Preferences::doubleQuotes() const
{
	return m_double_quotes;
}

//-----------------------------------------------------------------------------

int Preferences::singleQuotes() const
{
	return m_single_quotes;
}

//-----------------------------------------------------------------------------

bool Preferences::typewriterSounds() const
{
	return m_typewriter_sounds;
}

//-----------------------------------------------------------------------------

void Preferences::setAlwaysCenter(bool center)
{
	setValue(m_always_center, center);
}

//-----------------------------------------------------------------------------

void Preferences::setBlockCursor(bool block)
{
	setValue(m_block_cursor, block);
}

//-----------------------------------------------------------------------------

void Preferences::setSmoothFonts(bool smooth)
{
	setValue(m_smooth_fonts, smooth);
}

//-----------------------------------------------------------------------------

void Preferences::setSmartQuotes(bool quotes)
{
	setValue(m_smart_quotes, quotes);
}

//-----------------------------------------------------------------------------

void Preferences::setDoubleQuotes(int quotes)
{
	setValue(m_double_quotes, quotes);
}

//-----------------------------------------------------------------------------

void Preferences::setSingleQuotes(int quotes)
{
	setValue(m_single_quotes, quotes);
}

//-----------------------------------------------------------------------------

void Preferences::setTypewriterSounds(bool sounds)
{
	setValue(m_typewriter_sounds, sounds);
}

//-----------------------------------------------------------------------------

QString Preferences::sceneDivider() const
{
	return m_scene_divider;
}

//-----------------------------------------------------------------------------

void Preferences::setSceneDivider(const QString& divider)
{
	setValue(m_scene_divider, divider);
	SceneModel::setSceneDivider(m_scene_divider);
}

//-----------------------------------------------------------------------------

bool Preferences::savePositions() const
{
	return m_save_positions;
}

//-----------------------------------------------------------------------------

bool Preferences::writeByteOrderMark() const
{
	return m_write_bom;
}

//-----------------------------------------------------------------------------

RangedString Preferences::saveFormat() const
{
	return m_save_format;
}

//-----------------------------------------------------------------------------

void Preferences::setSavePositions(bool save)
{
	setValue(m_save_positions, save);
}

//-----------------------------------------------------------------------------

void Preferences::setWriteByteOrderMark(bool write_bom)
{
	setValue(m_write_bom, write_bom);
}

//-----------------------------------------------------------------------------

void Preferences::setSaveFormat(const QString& format)
{
	setValue(m_save_format, format);
}

//-----------------------------------------------------------------------------

bool Preferences::alwaysShowScrollBar() const
{
	return m_always_show_scrollbar;
}

//-----------------------------------------------------------------------------

void Preferences::setAlwaysShowScrollbar(bool show_scrollbar)
{
	setValue(m_always_show_scrollbar, show_scrollbar);
}

//-----------------------------------------------------------------------------

bool Preferences::alwaysShowHeader() const
{
	return m_always_show_header;
}

//-----------------------------------------------------------------------------

void Preferences::setAlwaysShowHeader(bool show_header)
{
	setValue(m_always_show_header, show_header);
}

//-----------------------------------------------------------------------------

bool Preferences::alwaysShowFooter() const
{
	return m_always_show_footer;
}

//-----------------------------------------------------------------------------

void Preferences::setAlwaysShowFooter(bool show_footer)
{
	setValue(m_always_show_footer, show_footer);
}

//-----------------------------------------------------------------------------

int Preferences::toolbarStyle() const
{
	return m_toolbar_style;
}

//-----------------------------------------------------------------------------

QStringList Preferences::toolbarActions() const
{
	return m_toolbar_actions;
}

//-----------------------------------------------------------------------------

void Preferences::setToolbarStyle(int style)
{
	setValue(m_toolbar_style, style);
}

//-----------------------------------------------------------------------------

void Preferences::setToolbarActions(const QStringList& actions)
{
	setValue(m_toolbar_actions, actions);
}

//-----------------------------------------------------------------------------

bool Preferences::highlightMisspelled() const
{
	return m_highlight_misspelled;
}

//-----------------------------------------------------------------------------

bool Preferences::ignoredWordsWithNumbers() const
{
	return m_ignore_numbers;
}

//-----------------------------------------------------------------------------

bool Preferences::ignoredUppercaseWords() const
{
	return m_ignore_uppercase;
}

//-----------------------------------------------------------------------------

QString Preferences::language() const
{
	return m_language;
}

//-----------------------------------------------------------------------------

void Preferences::setHighlightMisspelled(bool highlight)
{
	setValue(m_highlight_misspelled, highlight);
}

//-----------------------------------------------------------------------------

void Preferences::setIgnoreWordsWithNumbers(bool ignore)
{
	setValue(m_ignore_numbers, ignore);
}

//-----------------------------------------------------------------------------

void Preferences::setIgnoreUppercaseWords(bool ignore)
{
	setValue(m_ignore_uppercase, ignore);
}

//-----------------------------------------------------------------------------

void Preferences::setLanguage(const QString& language)
{
	setValue(m_language, DictionaryManager::instance().availableDictionary(language));
}

//-----------------------------------------------------------------------------

void Preferences::reload()
{
	QSettings settings;

	m_goal_type = settings.value("Goal/Type", 1).toInt();
	m_goal_minutes = settings.value("Goal/Minutes", 30).toInt();
	m_goal_words = settings.value("Goal/Words", 1000).toInt();
	m_goal_history = settings.value("Goal/History", true).toBool();
	m_goal_streaks = settings.value("Goal/Streaks", true).toBool();
	m_goal_streak_minimum = settings.value("Goal/StreakMinimum", 100).toInt();

	m_show_characters = settings.value("Stats/ShowCharacters", false).toBool();
	m_show_pages = settings.value("Stats/ShowPages", false).toBool();
	m_show_paragraphs = settings.value("Stats/ShowParagraphs", false).toBool();
	m_show_words = settings.value("Stats/ShowWords", true).toBool();

	m_page_type = settings.value("Stats/PageSizeType", 2).toInt();
	m_page_characters = settings.value("Stats/CharactersPerPage", 1500).toInt();
	m_page_paragraphs = settings.value("Stats/ParagraphsPerPage", 5).toInt();
	m_page_words = settings.value("Stats/WordsPerPage", 250).toInt();

	int old_wordcount_type = !settings.value("Stats/AccurateWordcount", true).toBool();
	QLocale::Language language = QLocale().language();
	if (language == QLocale::Chinese ||
			language == QLocale::Japanese ||
			language == QLocale::Khmer ||
			language == QLocale::Lao ||
			language == QLocale::Thai) {
		old_wordcount_type = 2;
	}
	m_wordcount_type = settings.value("Stats/WordcountType", old_wordcount_type).toInt();

	m_always_center = settings.value("Edit/AlwaysCenter", false).toBool();
	m_block_cursor = settings.value("Edit/BlockCursor", false).toBool();
	m_smooth_fonts = settings.value("Edit/SmoothFonts", true).toBool();
	m_smart_quotes = settings.value("Edit/SmartQuotes", true).toBool();
	m_double_quotes = settings.value("Edit/SmartDoubleQuotes", -1).toInt();
	m_single_quotes = settings.value("Edit/SmartSingleQuotes", -1).toInt();
	m_typewriter_sounds = settings.value("Edit/TypewriterSounds", false).toBool();

	m_scene_divider = settings.value("SceneList/Divider", QLatin1String("##")).toString();
	SceneModel::setSceneDivider(m_scene_divider);

	m_save_positions = settings.value("Save/RememberPositions", true).toBool();
	m_write_bom = settings.value("Save/WriteBOM", true).toBool();
	m_save_format = settings.value("Save/DefaultFormat", "odt").toString();

	m_toolbar_style = settings.value("Toolbar/Style", QApplication::style()->styleHint(QStyle::SH_ToolButtonStyle)).toInt();
	m_toolbar_actions = QStringList() << "New" << "Open" << "Save" << "|" << "Undo" << "Redo" << "|" << "Cut" << "Copy" << "Paste" << "|" << "Find" << "Replace" << "|" << "Themes";
	m_toolbar_actions = settings.value("Toolbar/Actions", m_toolbar_actions).toStringList();

	m_highlight_misspelled = settings.value("Spelling/HighlightMisspelled", true).toBool();
	m_ignore_numbers = settings.value("Spelling/IgnoreNumbers", true).toBool();
	m_ignore_uppercase = settings.value("Spelling/IgnoreUppercase", true).toBool();
	m_language = DictionaryManager::instance().availableDictionary(settings.value("Spelling/Language", QLocale().name()).toString());

	m_always_show_scrollbar = settings.value("View/AlwaysShowScrollbar",false).toBool();
	m_always_show_header = settings.value("View/AlwaysShowHeader",false).toBool();
	m_always_show_footer = settings.value("View/AlwaysShowFooter",false).toBool();

	DictionaryManager::instance().setDefaultLanguage(m_language);
	DictionaryManager::instance().setIgnoreNumbers(m_ignore_numbers);
	DictionaryManager::instance().setIgnoreUppercase(m_ignore_uppercase);
}

//-----------------------------------------------------------------------------

void Preferences::write()
{
	QSettings settings;

	settings.setValue("Goal/Type", m_goal_type.value());
	settings.setValue("Goal/Minutes", m_goal_minutes.value());
	settings.setValue("Goal/Words", m_goal_words.value());
	settings.setValue("Goal/History", m_goal_history);
	settings.setValue("Goal/Streaks", m_goal_streaks);
	settings.setValue("Goal/StreakMinimum", m_goal_streak_minimum.value());

	settings.setValue("Stats/ShowCharacters", m_show_characters);
	settings.setValue("Stats/ShowPages", m_show_pages);
	settings.setValue("Stats/ShowParagraphs", m_show_paragraphs);
	settings.setValue("Stats/ShowWords", m_show_words);

	settings.setValue("Stats/PageSizeType", m_page_type.value());
	settings.setValue("Stats/CharactersPerPage", m_page_characters.value());
	settings.setValue("Stats/ParagraphsPerPage", m_page_paragraphs.value());
	settings.setValue("Stats/WordsPerPage", m_page_words.value());

	settings.setValue("Stats/WordcountType", m_wordcount_type.value());

	settings.setValue("Edit/AlwaysCenter", m_always_center);
	settings.setValue("Edit/BlockCursor", m_block_cursor);
	settings.setValue("Edit/SmoothFonts", m_smooth_fonts);
	settings.setValue("Edit/SmartQuotes", m_smart_quotes);
	settings.setValue("Edit/SmartDoubleQuotes", m_double_quotes);
	settings.setValue("Edit/SmartSingleQuotes", m_single_quotes);
	settings.setValue("Edit/TypewriterSounds", m_typewriter_sounds);

	settings.setValue("SceneList/Divider", m_scene_divider);

	settings.setValue("Save/RememberPositions", m_save_positions);
	settings.setValue("Save/WriteBOM", m_write_bom);
	settings.setValue("Save/DefaultFormat", m_save_format.value());

	settings.setValue("Toolbar/Style", m_toolbar_style);
	settings.setValue("Toolbar/Actions", m_toolbar_actions);

	settings.setValue("Spelling/HighlightMisspelled", m_highlight_misspelled);
	settings.setValue("Spelling/IgnoreNumbers", m_ignore_numbers);
	settings.setValue("Spelling/IgnoreUppercase", m_ignore_uppercase);
	settings.setValue("Spelling/Language", m_language);

	settings.setValue("View/AlwaysShowScrollbar",m_always_show_scrollbar);
	settings.setValue("View/AlwaysShowHeader",m_always_show_header);
	settings.setValue("View/AlwaysShowFooter",m_always_show_footer);

	DictionaryManager::instance().addProviders();
	DictionaryManager::instance().setDefaultLanguage(m_language);
	DictionaryManager::instance().setIgnoreNumbers(m_ignore_numbers);
	DictionaryManager::instance().setIgnoreUppercase(m_ignore_uppercase);
}

//-----------------------------------------------------------------------------
