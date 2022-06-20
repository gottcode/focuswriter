/*
	SPDX-FileCopyrightText: 2008-2017 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "preferences.h"

#include "dictionary_manager.h"
#include "format_manager.h"
#include "scene_model.h"

#include <QApplication>
#include <QLocale>
#include <QSettings>
#include <QStyle>

//-----------------------------------------------------------------------------

Preferences::Preferences()
	: m_goal_type(0, 2)
	, m_goal_minutes(5, 1440)
	, m_goal_words(100, 100000)
	, m_goal_streak_minimum(1, 100)
	, m_page_type(0, 2)
	, m_page_characters(1, 10000)
	, m_page_paragraphs(1, 100)
	, m_page_words(1, 2000)
	, m_wordcount_type(0, 2)
	, m_save_format(FormatManager::types())
{
	forgetChanges();
}

//-----------------------------------------------------------------------------

Preferences::~Preferences()
{
	saveChanges();
}

//-----------------------------------------------------------------------------

void Preferences::setSceneDivider(const QString& divider)
{
	setValue(m_scene_divider, divider);
	SceneModel::setSceneDivider(m_scene_divider);
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
	const QLocale::Language language = QLocale().language();
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
	m_write_bom = settings.value("Save/WriteBOM", false).toBool();
	m_save_format = settings.value("Save/DefaultFormat", "odt").toString();

	m_toolbar_style = settings.value("Toolbar/Style", QApplication::style()->styleHint(QStyle::SH_ToolButtonStyle)).toInt();
	m_toolbar_actions = QStringList{ "New", "Open", "Save", "|", "Undo", "Redo", "|", "Cut", "Copy", "Paste", "|", "Find", "Replace", "|", "Themes" };
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
