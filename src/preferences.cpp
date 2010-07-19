/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary.h"

#include <QLocale>
#include <QSettings>

/*****************************************************************************/

Preferences::Preferences()
: m_changed(false) {
	QSettings settings;

	m_goal_type = settings.value("Goal/Type", 1).toInt();
	m_goal_minutes = settings.value("Goal/Minutes", 30).toInt();
	m_goal_words = settings.value("Goal/Words", 1000).toInt();

	m_show_characters = settings.value("Stats/ShowCharacters", true).toBool();
	m_show_pages = settings.value("Stats/ShowPages", true).toBool();
	m_show_paragraphs = settings.value("Stats/ShowParagraphs", true).toBool();
	m_show_words = settings.value("Stats/ShowWords", true).toBool();

	m_page_type = settings.value("Stats/PageSizeType", 0).toInt();
	m_page_characters = settings.value("Stats/CharactersPerPage", 1500).toInt();
	m_page_paragraphs = settings.value("Stats/ParagraphsPerPage", 5).toInt();
	m_page_words = settings.value("Stats/WordsPerPage", 250).toInt();

	m_accurate_wordcount = settings.value("Stats/AccurateWordcount", true).toBool();

	m_always_center = settings.value("Edit/AlwaysCenter", false).toBool();
	m_block_cursor = settings.value("Edit/BlockCursor", false).toBool();
	m_rich_text = settings.value("Edit/RichText", false).toBool();
	m_smooth_fonts = settings.value("Edit/SmoothFonts", true).toBool();

	m_auto_save = settings.value("Save/Auto", true).toBool();
	m_save_positions = settings.value("Save/RememberPositions", true).toBool();

	m_toolbar_style = settings.value("Toolbar/Style", Qt::ToolButtonTextUnderIcon).toInt();
	m_toolbar_actions = QStringList() << "New" << "Open" << "Save" << "|" << "Undo" << "Redo" << "|" << "Cut" << "Copy" << "Paste" << "|" << "Find" << "Replace";
	m_toolbar_actions = settings.value("Toolbar/Actions", m_toolbar_actions).toStringList();

	m_highlight_misspelled = settings.value("Spelling/HighlightMisspelled", true).toBool();
	m_ignore_numbers = settings.value("Spelling/IgnoreNumbers", true).toBool();
	m_ignore_uppercase = settings.value("Spelling/IgnoreUppercase", true).toBool();
	m_language = settings.value("Spelling/Language", QLocale::system().name()).toString();

	m_dictionary = new Dictionary;
	m_dictionary->setLanguage(m_language);
	m_dictionary->setIgnoreNumbers(m_ignore_numbers);
	m_dictionary->setIgnoreUppercase(m_ignore_uppercase);
}

/*****************************************************************************/

Preferences::~Preferences() {
	delete m_dictionary;
	if (!m_changed) {
		return;
	}

	QSettings settings;

	settings.setValue("Goal/Type", m_goal_type);
	settings.setValue("Goal/Minutes", m_goal_minutes);
	settings.setValue("Goal/Words", m_goal_words);

	settings.setValue("Stats/ShowCharacters", m_show_characters);
	settings.setValue("Stats/ShowPages", m_show_pages);
	settings.setValue("Stats/ShowParagraphs", m_show_paragraphs);
	settings.setValue("Stats/ShowWords", m_show_words);

	settings.setValue("Stats/PageSizeType", m_page_type);
	settings.setValue("Stats/CharactersPerPage", m_page_characters);
	settings.setValue("Stats/ParagraphsPerPage", m_page_paragraphs);
	settings.setValue("Stats/WordsPerPage", m_page_words);

	settings.setValue("Stats/AccurateWordcount", m_accurate_wordcount);

	settings.setValue("Edit/AlwaysCenter", m_always_center);
	settings.setValue("Edit/BlockCursor", m_block_cursor);
	settings.setValue("Edit/RichText", m_rich_text);
	settings.setValue("Edit/SmoothFonts", m_smooth_fonts);

	settings.setValue("Save/Auto", m_auto_save);
	settings.setValue("Save/RememberPositions", m_save_positions);

	settings.setValue("Toolbar/Style", m_toolbar_style);
	settings.setValue("Toolbar/Actions", m_toolbar_actions);

	settings.setValue("Spelling/HighlightMisspelled", m_highlight_misspelled);
	settings.setValue("Spelling/IgnoreNumbers", m_ignore_numbers);
	settings.setValue("Spelling/IgnoreUppercase", m_ignore_uppercase);
	settings.setValue("Spelling/Language", m_language);
}

/*****************************************************************************/

int Preferences::goalType() const {
	return m_goal_type;
}

/*****************************************************************************/

int Preferences::goalMinutes() const {
	return m_goal_minutes;
}

/*****************************************************************************/

int Preferences::goalWords() const {
	return m_goal_words;
}

/*****************************************************************************/

void Preferences::setGoalType(int goal) {
	m_goal_type = goal;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setGoalMinutes(int goal) {
	m_goal_minutes = goal;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setGoalWords(int goal) {
	m_goal_words = goal;
	m_changed = true;
}

/*****************************************************************************/

bool Preferences::showCharacters() const {
	return m_show_characters;
}

/*****************************************************************************/

bool Preferences::showPages() const {
	return m_show_pages;
}

/*****************************************************************************/

bool Preferences::showParagraphs() const {
	return m_show_paragraphs;
}

/*****************************************************************************/

bool Preferences::showWords() const {
	return m_show_words;
}

/*****************************************************************************/

void Preferences::setShowCharacters(bool show) {
	m_show_characters = show;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setShowPages(bool show) {
	m_show_pages = show;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setShowParagraphs(bool show) {
	m_show_paragraphs = show;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setShowWords(bool show) {
	m_show_words = show;
	m_changed = true;
}

/*****************************************************************************/

int Preferences::pageType() const {
	return m_page_type;
}

/*****************************************************************************/

int Preferences::pageCharacters() const {
	return m_page_characters;
}

/*****************************************************************************/

int Preferences::pageParagraphs() const {
	return m_page_paragraphs;
}

/*****************************************************************************/

int Preferences::pageWords() const {
	return m_page_words;
}

/*****************************************************************************/

void Preferences::setPageType(int type) {
	m_page_type = type;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setPageCharacters(int characters) {
	m_page_characters = characters;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setPageParagraphs(int paragraphs) {
	m_page_paragraphs = paragraphs;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setPageWords(int words) {
	m_page_words = words;
	m_changed = true;
}

/*****************************************************************************/

bool Preferences::accurateWordcount() const {
	return m_accurate_wordcount;
}

/*****************************************************************************/

void Preferences::setAccurateWordcount(bool accurate) {
	m_accurate_wordcount = accurate;
	m_changed = true;
}

/*****************************************************************************/

bool Preferences::alwaysCenter() const {
	return m_always_center;
}

/*****************************************************************************/

bool Preferences::blockCursor() const {
	return m_block_cursor;
}

/*****************************************************************************/

bool Preferences::richText() const {
	return m_rich_text;
}

/*****************************************************************************/

bool Preferences::smoothFonts() const {
	return m_smooth_fonts;
}

/*****************************************************************************/

void Preferences::setAlwaysCenter(bool center) {
	m_always_center = center;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setBlockCursor(bool block) {
	m_block_cursor = block;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setRichText(bool rich) {
	m_rich_text = rich;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setSmoothFonts(bool smooth) {
	m_smooth_fonts = smooth;
	m_changed = true;
}

/*****************************************************************************/

bool Preferences::autoSave() const {
	return m_auto_save;
}

/*****************************************************************************/

bool Preferences::savePositions() const {
	return m_save_positions;
}

/*****************************************************************************/

void Preferences::setAutoSave(bool save) {
	m_auto_save = save;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setSavePositions(bool save) {
	m_save_positions = save;
	m_changed = true;
}

/*****************************************************************************/

int Preferences::toolbarStyle() const {
	return m_toolbar_style;
}

/*****************************************************************************/

QStringList Preferences::toolbarActions() const {
	return m_toolbar_actions;
}

/*****************************************************************************/

void Preferences::setToolbarStyle(int style) {
	m_toolbar_style = style;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setToolbarActions(const QStringList& actions) {
	m_toolbar_actions = actions;
	m_changed = true;
}

/*****************************************************************************/

bool Preferences::highlightMisspelled() const {
	return m_highlight_misspelled;
}

/*****************************************************************************/

bool Preferences::ignoredWordsWithNumbers() const {
	return m_ignore_numbers;
}

/*****************************************************************************/

bool Preferences::ignoredUppercaseWords() const {
	return m_ignore_uppercase;
}

/*****************************************************************************/

QString Preferences::language() const {
	return m_language;
}

/*****************************************************************************/

void Preferences::setHighlightMisspelled(bool highlight) {
	m_highlight_misspelled = highlight;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setIgnoreWordsWithNumbers(bool ignore) {
	m_ignore_numbers = ignore;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setIgnoreUppercaseWords(bool ignore) {
	m_ignore_uppercase = ignore;
	m_changed = true;
}

/*****************************************************************************/

void Preferences::setLanguage(const QString& language) {
	m_language = language;
	m_changed = true;
}

/*****************************************************************************/
