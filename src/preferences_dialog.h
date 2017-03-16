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

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

class DailyProgress;
class ShortcutEdit;

#include <QDialog>
#include <QHash>
#include <QKeySequence>
class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QTreeWidget;

class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	PreferencesDialog(DailyProgress* daily_progress, QWidget* parent = 0);
	~PreferencesDialog();

public slots:
	virtual void accept();
	virtual void reject();

private slots:
	void goalHistoryToggled();
	void resetDailyGoal();
	void moveActionUp();
	void moveActionDown();
	void addSeparatorAction();
	void currentActionChanged(int action);
	void addLanguage();
	void removeLanguage();
	void selectedLanguageChanged(int index);
	void addWord();
	void removeWord();
	void selectedWordChanged();
	void wordEdited();
	void selectedShortcutChanged();
	void shortcutChanged();
	void shortcutDoubleClicked();

private:
	void highlightShortcutConflicts();
	QWidget* initGeneralTab();
	QWidget* initDailyGoalTab();
	QWidget* initStatisticsTab();
	QWidget* initSpellingTab();
	QWidget* initToolbarTab();
	QWidget* initShortcutsTab();

private:
	QTabWidget* m_tabs;

	QCheckBox* m_always_center;
	QCheckBox* m_block_cursor;
	QCheckBox* m_smooth_fonts;
	QCheckBox* m_smart_quotes;
	QComboBox* m_double_quotes;
	QComboBox* m_single_quotes;
	QCheckBox* m_typewriter_sounds;
	QLineEdit* m_scene_divider;
	QCheckBox* m_save_positions;
	QCheckBox* m_write_bom;
	QComboBox* m_save_format;
	QCheckBox* m_always_show_scrollbar;
	QCheckBox* m_always_show_header;
	QCheckBox* m_always_show_footer;

	DailyProgress* m_daily_progress;
	QRadioButton* m_option_none;
	QRadioButton* m_option_time;
	QRadioButton* m_option_wordcount;
	QSpinBox* m_time;
	QSpinBox* m_wordcount;
	QCheckBox* m_goal_history;
	QWidget* m_streak_minimum_label;
	QSpinBox* m_streak_minimum;
	QCheckBox* m_goal_streaks;

	QCheckBox* m_show_characters;
	QCheckBox* m_show_pages;
	QCheckBox* m_show_paragraphs;
	QCheckBox* m_show_words;
	QRadioButton* m_option_accurate_wordcount;
	QRadioButton* m_option_estimate_wordcount;
	QRadioButton* m_option_singlechar_wordcount;
	QRadioButton* m_option_characters;
	QRadioButton* m_option_paragraphs;
	QRadioButton* m_option_words;
	QSpinBox* m_page_characters;
	QSpinBox* m_page_paragraphs;
	QSpinBox* m_page_words;

	QCheckBox* m_highlight_misspelled;
	QCheckBox* m_ignore_uppercase;
	QCheckBox* m_ignore_numbers;
	QComboBox* m_languages;
	QLineEdit* m_word;
	QListWidget* m_personal_dictionary;
	QPushButton* m_add_language_button;
	QPushButton* m_remove_language_button;
	QPushButton* m_add_word_button;
	QPushButton* m_remove_word_button;
	QStringList m_uninstalled;

	QComboBox* m_toolbar_style;
	QListWidget* m_toolbar_actions;
	QPushButton* m_move_up_button;
	QPushButton* m_move_down_button;

	QTreeWidget* m_shortcuts;
	ShortcutEdit* m_shortcut_edit;
	QHash<QString, QKeySequence> m_new_shortcuts;
	bool m_shortcut_conflicts;
};

#endif
