/*
	SPDX-FileCopyrightText: 2008-2017 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_PREFERENCES_DIALOG_H
#define FOCUSWRITER_PREFERENCES_DIALOG_H

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
	explicit PreferencesDialog(DailyProgress* daily_progress, QWidget* parent = nullptr);
	~PreferencesDialog();

public Q_SLOTS:
	void accept() override;
	void reject() override;

private Q_SLOTS:
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

#endif // FOCUSWRITER_PREFERENCES_DIALOG_H
