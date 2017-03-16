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

#include "preferences_dialog.h"

#include "action_manager.h"
#include "daily_progress.h"
#include "dictionary_manager.h"
#include "format_manager.h"
#include "locale_dialog.h"
#include "preferences.h"
#include "shortcut_edit.h"
#include "smart_quotes.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QtZipReader>

//-----------------------------------------------------------------------------

namespace
{
	QWidget* makeScrollable(QWidget* tab)
	{
		QScrollArea* area = new QScrollArea(tab->parentWidget());
		area->setFrameStyle(QFrame::NoFrame);
		area->setWidget(tab);
		area->setWidgetResizable(true);

		area->setBackgroundRole(QPalette::Link);
		QPalette p = area->palette();
		p.setColor(area->backgroundRole(), Qt::transparent);
		area->setPalette(p);

		return area;
	}
}

//-----------------------------------------------------------------------------

PreferencesDialog::PreferencesDialog(DailyProgress* daily_progress, QWidget* parent) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_daily_progress(daily_progress),
	m_shortcut_conflicts(false)
{
	setWindowTitle(tr("Preferences"));

	m_tabs = new QTabWidget(this);
	m_tabs->addTab(initGeneralTab(), tr("General"));
	m_tabs->addTab(initDailyGoalTab(), tr("Daily Goal"));
	m_tabs->addTab(initStatisticsTab(), tr("Statistics"));
	m_tabs->addTab(initSpellingTab(), tr("Spell Checking"));
	m_tabs->addTab(initToolbarTab(), tr("Toolbar"));
	m_tabs->addTab(initShortcutsTab(), tr("Shortcuts"));
	m_tabs->setUsesScrollButtons(false);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_tabs);
	layout->addWidget(buttons);

	// Load settings
	switch (Preferences::instance().goalType()) {
	case 1:
		m_option_time->setChecked(true);
		break;
	case 2:
		m_option_wordcount->setChecked(true);
		break;
	default:
		m_option_none->setChecked(true);
		break;
	}
	m_time->setValue(Preferences::instance().goalMinutes());
	m_wordcount->setValue(Preferences::instance().goalWords());

	m_goal_history->setChecked(Preferences::instance().goalHistory());
	m_goal_streaks->setChecked(Preferences::instance().goalStreaks());
	m_streak_minimum->setValue(Preferences::instance().goalStreakMinimum());

	m_show_characters->setChecked(Preferences::instance().showCharacters());
	m_show_pages->setChecked(Preferences::instance().showPages());
	m_show_paragraphs->setChecked(Preferences::instance().showParagraphs());
	m_show_words->setChecked(Preferences::instance().showWords());

	switch (Preferences::instance().pageType()) {
	case 1:
		m_option_paragraphs->setChecked(true);
		break;
	case 2:
		m_option_words->setChecked(true);
		break;
	default:
		m_option_characters->setChecked(true);
		break;
	}
	m_page_characters->setValue(Preferences::instance().pageCharacters());
	m_page_paragraphs->setValue(Preferences::instance().pageParagraphs());
	m_page_words->setValue(Preferences::instance().pageWords());

	switch (Preferences::instance().wordcountType()) {
	case 1:
		m_option_estimate_wordcount->setChecked(true);
		break;
	case 2:
		m_option_singlechar_wordcount->setChecked(true);
		break;
	default:
		m_option_accurate_wordcount->setChecked(true);
		break;
	}

	m_always_center->setChecked(Preferences::instance().alwaysCenter());
	m_block_cursor->setChecked(Preferences::instance().blockCursor());
	m_smooth_fonts->setChecked(Preferences::instance().smoothFonts());
	m_smart_quotes->setChecked(Preferences::instance().smartQuotes());
	m_double_quotes->setCurrentIndex(Preferences::instance().doubleQuotes());
	m_single_quotes->setCurrentIndex(Preferences::instance().singleQuotes());
	m_typewriter_sounds->setChecked(Preferences::instance().typewriterSounds());

	m_scene_divider->setText(Preferences::instance().sceneDivider());

	m_save_positions->setChecked(Preferences::instance().savePositions());
	m_save_format->setCurrentIndex(m_save_format->findData(Preferences::instance().saveFormat().value()));
	m_write_bom->setChecked(Preferences::instance().writeByteOrderMark());

	m_always_show_scrollbar->setChecked(Preferences::instance().alwaysShowScrollBar());
	m_always_show_header->setChecked(Preferences::instance().alwaysShowHeader());
	m_always_show_footer->setChecked(Preferences::instance().alwaysShowFooter());

	m_highlight_misspelled->setChecked(Preferences::instance().highlightMisspelled());
	m_ignore_numbers->setChecked(Preferences::instance().ignoredWordsWithNumbers());
	m_ignore_uppercase->setChecked(Preferences::instance().ignoredUppercaseWords());
	int index = m_languages->findData(Preferences::instance().language());
	if (index != -1) {
		m_languages->setCurrentIndex(index);
	}

	int style = m_toolbar_style->findData(Preferences::instance().toolbarStyle());
	if (style == -1) {
		style = m_toolbar_style->findData(Qt::ToolButtonTextUnderIcon);
	}
	m_toolbar_style->setCurrentIndex(style);
	QStringList actions = Preferences::instance().toolbarActions();
	int pos = 0;
	for (const QString& action : actions) {
		QString text = action;
		bool checked = !text.startsWith("^");
		if (!checked) {
			text.remove(0, 1);
		}

		QListWidgetItem* item = 0;
		if (text != "|") {
			int count = m_toolbar_actions->count();
			for (int i = pos; i < count; ++i) {
				if (m_toolbar_actions->item(i)->data(Qt::UserRole).toString() == text) {
					item = m_toolbar_actions->takeItem(i);
					break;
				}
			}
		} else if (checked) {
			item = new QListWidgetItem(QString(20, QChar('-')));
			item->setData(Qt::UserRole, "|");
		}

		if (item != 0) {
			item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
			m_toolbar_actions->insertItem(pos, item);
			pos++;
		}
	}
	m_toolbar_actions->setCurrentRow(0);

	resize(QSettings().value("Preferences/Size", QSize(650, 560)).toSize());
}

//-----------------------------------------------------------------------------

PreferencesDialog::~PreferencesDialog()
{
	QSettings().setValue("Preferences/Size", size());
}

//-----------------------------------------------------------------------------

void PreferencesDialog::accept()
{
	// Confirm close even with shortcut conflicts
	if (m_shortcut_conflicts) {
		m_tabs->setCurrentIndex(5);
		if (QMessageBox::question(this,
				tr("Question"),
				tr("One or more shortcuts conflict. Do you wish to proceed?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No) == QMessageBox::No) {
			return;
		}
	}

	// Save settings
	if (m_option_time->isChecked()) {
		Preferences::instance().setGoalType(1);
	} else if (m_option_wordcount->isChecked()) {
		Preferences::instance().setGoalType(2);
	} else {
		Preferences::instance().setGoalType(0);
	}
	Preferences::instance().setGoalMinutes(m_time->value());
	Preferences::instance().setGoalWords(m_wordcount->value());
	Preferences::instance().setGoalHistory(m_goal_history->isChecked());
	Preferences::instance().setGoalStreaks(m_goal_streaks->isChecked());
	Preferences::instance().setGoalStreakMinimum(m_streak_minimum->value());

	Preferences::instance().setShowCharacters(m_show_characters->isChecked());
	Preferences::instance().setShowPages(m_show_pages->isChecked());
	Preferences::instance().setShowParagraphs(m_show_paragraphs->isChecked());
	Preferences::instance().setShowWords(m_show_words->isChecked());

	if (m_option_paragraphs->isChecked()) {
		Preferences::instance().setPageType(1);
	} else if (m_option_words->isChecked()) {
		Preferences::instance().setPageType(2);
	} else {
		Preferences::instance().setPageType(0);
	}
	Preferences::instance().setPageCharacters(m_page_characters->value());
	Preferences::instance().setPageParagraphs(m_page_paragraphs->value());
	Preferences::instance().setPageWords(m_page_words->value());

	if (m_option_accurate_wordcount->isChecked()) {
		Preferences::instance().setWordcountType(0);
	} else if (m_option_estimate_wordcount->isChecked()) {
		Preferences::instance().setWordcountType(1);
	} else {
		Preferences::instance().setWordcountType(2);
	}

	Preferences::instance().setAlwaysCenter(m_always_center->isChecked());
	Preferences::instance().setBlockCursor(m_block_cursor->isChecked());
	Preferences::instance().setSmoothFonts(m_smooth_fonts->isChecked());
	Preferences::instance().setSmartQuotes(m_smart_quotes->isChecked());
	Preferences::instance().setDoubleQuotes(m_double_quotes->currentIndex());
	Preferences::instance().setSingleQuotes(m_single_quotes->currentIndex());
	Preferences::instance().setTypewriterSounds(m_typewriter_sounds->isChecked());

	Preferences::instance().setSceneDivider(m_scene_divider->text());

	Preferences::instance().setSavePositions(m_save_positions->isChecked());
	Preferences::instance().setWriteByteOrderMark(m_write_bom->isChecked());
	Preferences::instance().setSaveFormat(m_save_format->itemData(m_save_format->currentIndex()).toString());

	Preferences::instance().setAlwaysShowScrollbar(m_always_show_scrollbar->isChecked());
	Preferences::instance().setAlwaysShowHeader(m_always_show_header->isChecked());
	Preferences::instance().setAlwaysShowFooter(m_always_show_footer->isChecked());

	Preferences::instance().setToolbarStyle(m_toolbar_style->itemData(m_toolbar_style->currentIndex()).toInt());
	QStringList actions;
	int count = m_toolbar_actions->count();
	for (int i = 0; i < count; ++i) {
		QListWidgetItem* item = m_toolbar_actions->item(i);
		QString action = (item->checkState() == Qt::Unchecked ? "^" : "") + item->data(Qt::UserRole).toString();
		if (action != "^|") {
			actions.append(action);
		}
	}
	Preferences::instance().setToolbarActions(actions);

	ActionManager::instance()->setShortcuts(m_new_shortcuts);

	// Uninstall languages
	for (const QString& language : m_uninstalled) {
		QFile::remove("dict:" + language + ".aff");
		QFile::remove("dict:" + language + ".dic");
	}

	// Install languages
	QString path = DictionaryManager::path() + "/install/";
	QString new_path = DictionaryManager::installedPath() + "/";
	QDir dir(path);
	QStringList files = dir.entryList(QDir::Files);
	for (const QString& file : files) {
		QFile::remove(new_path + file);
		QFile::rename(path + file, new_path + file);
	}
	dir.cdUp();
	dir.rmdir("install");

	// Set dictionary
	Preferences::instance().setHighlightMisspelled(m_highlight_misspelled->isChecked());
	Preferences::instance().setIgnoreWordsWithNumbers(m_ignore_numbers->isChecked());
	Preferences::instance().setIgnoreUppercaseWords(m_ignore_uppercase->isChecked());
	if (m_languages->count()) {
		Preferences::instance().setLanguage(m_languages->itemData(m_languages->currentIndex()).toString());
	} else {
		Preferences::instance().setLanguage(QString());
	}

	Preferences::instance().saveChanges();

	// Save personal dictionary
	QStringList words;
	for (int i = 0; i < m_personal_dictionary->count(); ++i) {
		words.append(m_personal_dictionary->item(i)->text());
	}
	DictionaryManager::instance().setPersonal(words);

	QDialog::accept();
}

//-----------------------------------------------------------------------------

void PreferencesDialog::reject()
{
	if (!QDir(DictionaryManager::path() + "/install/").removeRecursively()) {
		qWarning("Failed to clean up dictionary install path");
	}
	QDialog::reject();
}

//-----------------------------------------------------------------------------

void PreferencesDialog::goalHistoryToggled()
{
	m_goal_streaks->setEnabled(m_goal_history->isChecked());
	m_streak_minimum->setEnabled(m_goal_streaks->isChecked() && m_goal_streaks->isEnabled());
	m_streak_minimum_label->setEnabled(m_goal_streaks->isChecked() && m_goal_streaks->isEnabled());
}

//-----------------------------------------------------------------------------

void PreferencesDialog::resetDailyGoal()
{
	if (QMessageBox::question(this,
			tr("Question"),
			tr("Reset daily progress for today to zero?"),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No)
		== QMessageBox::Yes)
	{
		m_daily_progress->resetToday();
	}
}

//-----------------------------------------------------------------------------

void PreferencesDialog::moveActionUp()
{
	int from = m_toolbar_actions->currentRow();
	int to = from - 1;
	if (from > 0) {
		m_toolbar_actions->insertItem(to, m_toolbar_actions->takeItem(from));
		m_toolbar_actions->setCurrentRow(to);
	}
}

//-----------------------------------------------------------------------------

void PreferencesDialog::moveActionDown()
{
	int from = m_toolbar_actions->currentRow();
	int to = from + 1;
	if (to < m_toolbar_actions->count()) {
		m_toolbar_actions->insertItem(to, m_toolbar_actions->takeItem(from));
		m_toolbar_actions->setCurrentRow(to);
	}
}

//-----------------------------------------------------------------------------

void PreferencesDialog::addSeparatorAction()
{
	QListWidgetItem* item = new QListWidgetItem(QString(20, QChar('-')));
	item->setCheckState(Qt::Checked);
	item->setData(Qt::UserRole, "|");
	m_toolbar_actions->insertItem(m_toolbar_actions->currentRow(), item);
}

//-----------------------------------------------------------------------------

void PreferencesDialog::currentActionChanged(int action)
{
	if (action != -1) {
		m_move_up_button->setEnabled(action > 0);
		m_move_down_button->setEnabled((action + 1) < m_toolbar_actions->count());
	}
}

//-----------------------------------------------------------------------------

void PreferencesDialog::addLanguage()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Select Dictionary"), QDir::homePath());
	if (path.isEmpty()) {
		return;
	}

	// File lists
	QStringList aff_files;
	QStringList dic_files;
	QStringList files;
	QStringList dictionaries;

	// Open archive
	QtZipReader zip(path);
	if (!zip.isReadable()) {
		QMessageBox::warning(this, tr("Sorry"), tr("Unable to open archive."));
		return;
	}

	// List files
	QStringList entries = zip.fileList();
	for (int i = 0; i < entries.count(); ++i) {
		QString name = entries.at(i);
		if (name.endsWith(".aff")) {
			aff_files += name;
		} else if (name.endsWith(".dic")) {
			dic_files += name;
		}
	}

	// Find Hunspell dictionary files
	for (const QString& dic : dic_files) {
		QString aff = dic;
		aff.replace(".dic", ".aff");
		if (aff_files.contains(aff)) {
			files += dic;
			files += aff;
			QString dictionary = dic.section('/', -1);
			dictionary.chop(4);
			dictionaries += dictionary;
		}
	}

	// Check for dictionaries
	if (!dictionaries.isEmpty()) {
		// Extract files
		QDir dir(DictionaryManager::path());
		dir.mkdir("install");
		QString install = dir.absoluteFilePath("install") + "/";
		for (const QString& file : files) {
			// Ignore path for Hunspell dictionaries
			QString filename = file;
			filename = filename.section('/', -1);
			filename.replace(QChar('-'), QChar('_'));

			QFile out(install + filename);
			if (out.open(QIODevice::WriteOnly)) {
				out.write(zip.fileData(file));
			}
		}

		// Add to language selection
		QString dictionary_path = DictionaryManager::path() + "/install/";
		QString dictionary_new_path = DictionaryManager::installedPath() + "/";
		for (const QString& dictionary : dictionaries) {
			QString language = dictionary;
			language.replace(QChar('-'), QChar('_'));
			QString name = LocaleDialog::languageName(language);

			// Prompt user about replacing duplicate languages
			QString aff_file = dictionary_path + dictionary + ".aff";
			QString dic_file = dictionary_path + dictionary + ".dic";
			QString new_aff_file = dictionary_new_path + language + ".aff";
			QString new_dic_file = dictionary_new_path + language + ".dic";

			if (!m_uninstalled.contains(language) && (QFile::exists(new_aff_file) || QFile::exists(new_dic_file))) {
				if (QMessageBox::question(this, tr("Question"), tr("The dictionary \"%1\" already exists. Do you want to replace it?").arg(name), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
					QFile::remove(aff_file);
					QFile::remove(dic_file);
				}
				continue;
			}

			m_languages->addItem(name, language);
			m_languages->setCurrentIndex(m_languages->count() - 1);
		}
		m_languages->model()->sort(0);
	} else {
		QMessageBox::warning(this, tr("Sorry"), tr("The archive does not contain a usable dictionary."));
	}

	// Close archive
	zip.close();
}

//-----------------------------------------------------------------------------

void PreferencesDialog::removeLanguage()
{
	int index = m_languages->currentIndex();
	if (index == -1) {
		return;
	}
	if (QMessageBox::question(this, tr("Question"), tr("Remove current dictionary?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		m_uninstalled.append(m_languages->itemData(index).toString());
		m_languages->removeItem(index);
	}
}

//-----------------------------------------------------------------------------

void PreferencesDialog::selectedLanguageChanged(int index)
{
	if (index != -1) {
		QFileInfo info("dict:" + m_languages->itemData(index).toString() + ".dic");
		m_remove_language_button->setEnabled(info.canonicalFilePath().startsWith(DictionaryManager::installedPath()));
	}
}

//-----------------------------------------------------------------------------

void PreferencesDialog::addWord()
{
	QString word = m_word->text();
	m_word->clear();
	int row;
	for (row = 0; row < m_personal_dictionary->count(); ++row) {
		if (m_personal_dictionary->item(row)->text().localeAwareCompare(word) > 0) {
			break;
		}
	}
	m_personal_dictionary->insertItem(row, word);
}

//-----------------------------------------------------------------------------

void PreferencesDialog::removeWord()
{
	delete m_personal_dictionary->selectedItems().first();
	m_personal_dictionary->clearSelection();
}

//-----------------------------------------------------------------------------

void PreferencesDialog::selectedWordChanged()
{
	m_remove_word_button->setDisabled(m_personal_dictionary->selectedItems().isEmpty());
}

//-----------------------------------------------------------------------------

void PreferencesDialog::wordEdited()
{
	QString word = m_word->text();
	m_add_word_button->setEnabled(!word.isEmpty() && m_personal_dictionary->findItems(word, Qt::MatchExactly).isEmpty());
}

//-----------------------------------------------------------------------------

void PreferencesDialog::selectedShortcutChanged()
{
	m_shortcut_edit->setEnabled(m_shortcuts->currentItem() != 0);
	if (!m_shortcuts->currentItem()) {
		m_shortcut_edit->blockSignals(true);
		m_shortcut_edit->setShortcut(QKeySequence(), QKeySequence());
		m_shortcut_edit->blockSignals(false);
		return;
	}

	// Set shortcut in editor
	QString name = m_shortcuts->currentItem()->text(2);
	QKeySequence shortcut = m_new_shortcuts.value(name, ActionManager::instance()->shortcut(name));
	m_shortcut_edit->blockSignals(true);
	m_shortcut_edit->setShortcut(shortcut, ActionManager::instance()->defaultShortcut(name));
	m_shortcut_edit->blockSignals(false);
}

//-----------------------------------------------------------------------------

void PreferencesDialog::shortcutChanged()
{
	if (!m_shortcuts->currentItem()) {
		return;
	}

	// Find old shortcut
	QString name = m_shortcuts->currentItem()->text(2);
	QKeySequence old_shortcut = m_new_shortcuts.value(name, ActionManager::instance()->shortcut(name));
	QKeySequence shortcut = m_shortcut_edit->shortcut();
	if (shortcut == old_shortcut) {
		return;
	}

	// Update shortcut
	m_new_shortcuts[name] = shortcut;
	m_shortcuts->currentItem()->setText(1, shortcut.toString(QKeySequence::NativeText));
	highlightShortcutConflicts();
}

//-----------------------------------------------------------------------------

void PreferencesDialog::shortcutDoubleClicked()
{
	m_shortcut_edit->setFocus();
}

//-----------------------------------------------------------------------------

void PreferencesDialog::highlightShortcutConflicts()
{
	m_shortcut_conflicts = false;
	QFont conflict = font();
	conflict.setBold(true);

	QMap<QKeySequence, QTreeWidgetItem*> shortcuts;
	for (int i = 0, count = m_shortcuts->topLevelItemCount(); i < count; ++i) {
		// Reset font and highlight
		QTreeWidgetItem* item = m_shortcuts->topLevelItem(i);
		item->setForeground(1, palette().foreground());
		item->setFont(1, font());

		// Find shortcut
		QString name = item->text(2);
		QKeySequence shortcut = m_new_shortcuts.value(name, ActionManager::instance()->shortcut(name));
		if (shortcut.isEmpty() || (shortcut == Qt::Key_unknown)) {
			continue;
		}

		// Highlight conflict
		if (shortcuts.contains(shortcut)) {
			m_shortcut_conflicts = true;
			item->setForeground(1, Qt::red);
			item->setFont(1,conflict);
			shortcuts[shortcut]->setForeground(1, Qt::red);
			shortcuts[shortcut]->setFont(1, conflict);
		}
		shortcuts[shortcut] = item;
	}
}

//-----------------------------------------------------------------------------

QWidget* PreferencesDialog::initGeneralTab()
{
	QWidget* tab = new QWidget(this);

	// Create edit options
	QGroupBox* edit_group = new QGroupBox(tr("Editing"), tab);

	m_always_center = new QCheckBox(tr("Always vertically center"), edit_group);
	m_block_cursor = new QCheckBox(tr("Block insertion cursor"), edit_group);
	m_smooth_fonts = new QCheckBox(tr("Smooth fonts"), edit_group);
	m_typewriter_sounds = new QCheckBox(tr("Typewriter sounds"), edit_group);

	m_smart_quotes = new QCheckBox(tr("Smart quotes:"), edit_group);
	m_double_quotes = new QComboBox(edit_group);
	m_double_quotes->setEnabled(false);
	m_single_quotes = new QComboBox(edit_group);
	m_single_quotes->setEnabled(false);
	int count = SmartQuotes::count();
	for (int i = 0; i < count; ++i) {
		m_double_quotes->addItem(SmartQuotes::quoteString(tr("Double"), i));
		m_single_quotes->addItem(SmartQuotes::quoteString(tr("Single"), i));
	}
	m_double_quotes->setMaxVisibleItems(count);
	m_single_quotes->setMaxVisibleItems(count);
	connect(m_smart_quotes, SIGNAL(toggled(bool)), m_double_quotes, SLOT(setEnabled(bool)));
	connect(m_smart_quotes, SIGNAL(toggled(bool)), m_single_quotes, SLOT(setEnabled(bool)));

	QHBoxLayout* quotes_layout = new QHBoxLayout;
	quotes_layout->addWidget(m_smart_quotes);
	quotes_layout->addWidget(m_double_quotes);
	quotes_layout->addWidget(m_single_quotes);
	quotes_layout->addStretch();

	QVBoxLayout* edit_layout = new QVBoxLayout(edit_group);
	edit_layout->addWidget(m_always_center);
	edit_layout->addWidget(m_block_cursor);
	edit_layout->addWidget(m_smooth_fonts);
	edit_layout->addLayout(quotes_layout);
	edit_layout->addWidget(m_typewriter_sounds);

	// Create section options
	QGroupBox* scene_group = new QGroupBox(tr("Scenes"), tab);

	m_scene_divider = new QLineEdit(scene_group);

	QFormLayout* scene_layout = new QFormLayout(scene_group);
	scene_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	scene_layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
	scene_layout->addRow(tr("Divider:"), m_scene_divider);

	// Create save options
	QGroupBox* save_group = new QGroupBox(tr("Saving"), tab);

	m_save_positions = new QCheckBox(tr("Remember cursor position"), save_group);
	m_write_bom = new QCheckBox(tr("Write byte order mark in plain text files"), save_group);

	QLabel* save_format_label = new QLabel(tr("Default format:"), save_group);
	m_save_format = new QComboBox(save_group);
	QStringList types = Preferences::instance().saveFormat().allowedValues();
	for (const QString& type : types) {
		m_save_format->addItem(FormatManager::filter(type), type);
	}

	QHBoxLayout* save_format_layout = new QHBoxLayout;
	save_format_layout->setMargin(0);
	save_format_layout->addWidget(save_format_label);
	save_format_layout->addWidget(m_save_format);
	save_format_layout->addStretch();

	QVBoxLayout* save_layout = new QVBoxLayout(save_group);
	save_layout->addWidget(m_save_positions);
	save_layout->addWidget(m_write_bom);
	save_layout->addLayout(save_format_layout);

	// Create view options
	QGroupBox* view_group = new QGroupBox(tr("User Interface"), tab);

	m_always_show_scrollbar = new QCheckBox(tr("Always show scrollbar"), view_group);
	m_always_show_header = new QCheckBox(tr("Always show top bar"), view_group);
	m_always_show_footer = new QCheckBox(tr("Always show bottom bar"), view_group);

	QVBoxLayout* view_layout = new QVBoxLayout(view_group);
	view_layout->addWidget(m_always_show_scrollbar);
	view_layout->addWidget(m_always_show_header);
	view_layout->addWidget(m_always_show_footer);

	// Lay out general options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addWidget(edit_group);
	layout->addWidget(scene_group);
	layout->addWidget(save_group);
	layout->addWidget(view_group);
	layout->addStretch();

	return makeScrollable(tab);
}

//-----------------------------------------------------------------------------

QWidget* PreferencesDialog::initDailyGoalTab()
{
	QWidget* tab = new QWidget(this);

	// Create goal options
	m_option_none = new QRadioButton(tr("None"), tab);

	m_option_time = new QRadioButton(tr("Minutes:"), tab);
	m_time = new QSpinBox(tab);
	m_time->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_time->setRange(Preferences::instance().goalMinutes().minimumValue(), Preferences::instance().goalMinutes().maximumValue());
	m_time->setSingleStep(5);
	m_time->setEnabled(false);

	m_option_wordcount = new QRadioButton(tr("Words:"), tab);
	m_wordcount = new QSpinBox(tab);
	m_wordcount->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_wordcount->setRange(Preferences::instance().goalWords().minimumValue(), Preferences::instance().goalWords().maximumValue());
	m_wordcount->setSingleStep(100);
	m_wordcount->setEnabled(false);

	connect(m_option_none, SIGNAL(toggled(bool)), m_time, SLOT(setDisabled(bool)));
	connect(m_option_none, SIGNAL(toggled(bool)), m_wordcount, SLOT(setDisabled(bool)));

	connect(m_option_time, SIGNAL(toggled(bool)), m_time, SLOT(setEnabled(bool)));
	connect(m_option_time, SIGNAL(toggled(bool)), m_wordcount, SLOT(setDisabled(bool)));

	connect(m_option_wordcount, SIGNAL(toggled(bool)), m_time, SLOT(setDisabled(bool)));
	connect(m_option_wordcount, SIGNAL(toggled(bool)), m_wordcount, SLOT(setEnabled(bool)));

	QPushButton* reset_today_button = new QPushButton(tr("Reset Today"), tab);
	connect(reset_today_button, SIGNAL(clicked()), this, SLOT(resetDailyGoal()));

	QGridLayout* goal_layout = new QGridLayout;
	goal_layout->setColumnStretch(2, 1);
	goal_layout->addWidget(m_option_none, 0, 0);
	goal_layout->addWidget(m_option_time, 1, 0);
	goal_layout->addWidget(m_time, 1, 1);
	goal_layout->addWidget(m_option_wordcount, 2, 0);
	goal_layout->addWidget(m_wordcount, 2, 1);
	goal_layout->addWidget(reset_today_button, 3, 0, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);

	// Create history options
	QGroupBox* history_group = new QGroupBox(tr("History"), tab);

	m_goal_history = new QCheckBox(tr("Remember history"), history_group);
	connect(m_goal_history, SIGNAL(toggled(bool)), this, SLOT(goalHistoryToggled()));

	m_goal_streaks = new QCheckBox(tr("Show streaks"), history_group);
	m_goal_streaks->setEnabled(false);
	connect(m_goal_streaks, SIGNAL(toggled(bool)), this, SLOT(goalHistoryToggled()));

	m_streak_minimum = new QSpinBox(history_group);
	m_streak_minimum->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_streak_minimum->setRange(Preferences::instance().goalStreakMinimum().minimumValue(), Preferences::instance().goalStreakMinimum().maximumValue());
	m_streak_minimum->setSuffix(QLocale().percent());
	m_streak_minimum->setEnabled(false);

	QFormLayout* history_layout = new QFormLayout(history_group);
	history_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	history_layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
	history_layout->addRow(m_goal_history);
	history_layout->addRow(m_goal_streaks);
	history_layout->addRow(tr("Minimum progress for streaks:"), m_streak_minimum);

	m_streak_minimum_label = history_layout->labelForField(m_streak_minimum);
	m_streak_minimum_label->setEnabled(false);

	// Lay out daily goal options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addLayout(goal_layout);
	layout->addWidget(history_group);
	layout->addStretch();

	return makeScrollable(tab);
}

//-----------------------------------------------------------------------------

QWidget* PreferencesDialog::initStatisticsTab()
{
	QWidget* tab = new QWidget(this);

	// Create statistics options
	m_show_words = new QCheckBox(tr("Word count"), tab);
	m_show_pages = new QCheckBox(tr("Page count"), tab);
	m_show_paragraphs = new QCheckBox(tr("Paragraph count"), tab);
	m_show_characters = new QCheckBox(tr("Character count"), tab);

	QVBoxLayout* counts_layout = new QVBoxLayout;
	counts_layout->addWidget(m_show_words);
	counts_layout->addWidget(m_show_pages);
	counts_layout->addWidget(m_show_paragraphs);
	counts_layout->addWidget(m_show_characters);

	// Create word count algorithm options
	QGroupBox* wordcount_group = new QGroupBox(tr("Word Count Algorithm"), this);

	m_option_accurate_wordcount = new QRadioButton(tr("Detect word boundaries"), wordcount_group);
	m_option_estimate_wordcount = new QRadioButton(tr("Divide character count by six"), wordcount_group);
	m_option_singlechar_wordcount = new QRadioButton(tr("Count each letter as a word"), wordcount_group);

	QVBoxLayout* wordcount_layout = new QVBoxLayout(wordcount_group);
	wordcount_layout->addWidget(m_option_accurate_wordcount);
	wordcount_layout->addWidget(m_option_estimate_wordcount);
	wordcount_layout->addWidget(m_option_singlechar_wordcount);

	// Create page count algorithm options
	QGroupBox* page_group = new QGroupBox(tr("Page Count Algorithm"), tab);

	m_option_characters = new QRadioButton(tr("Characters:"), page_group);
	m_page_characters = new QSpinBox(page_group);
	m_page_characters->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_page_characters->setRange(Preferences::instance().pageCharacters().minimumValue(), Preferences::instance().pageCharacters().maximumValue());
	m_page_characters->setSingleStep(250);
	m_page_characters->setEnabled(false);

	m_option_paragraphs = new QRadioButton(tr("Paragraphs:"), page_group);
	m_page_paragraphs = new QSpinBox(page_group);
	m_page_paragraphs->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_page_paragraphs->setRange(Preferences::instance().pageParagraphs().minimumValue(), Preferences::instance().pageParagraphs().maximumValue());
	m_page_paragraphs->setSingleStep(1);
	m_page_paragraphs->setEnabled(false);

	m_option_words = new QRadioButton(tr("Words:"), page_group);
	m_page_words = new QSpinBox(page_group);
	m_page_words->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_page_words->setRange(Preferences::instance().pageWords().minimumValue(), Preferences::instance().pageWords().maximumValue());
	m_page_words->setSingleStep(50);
	m_page_words->setEnabled(false);

	connect(m_option_characters, SIGNAL(toggled(bool)), m_page_characters, SLOT(setEnabled(bool)));
	connect(m_option_characters, SIGNAL(toggled(bool)), m_page_paragraphs, SLOT(setDisabled(bool)));
	connect(m_option_characters, SIGNAL(toggled(bool)), m_page_words, SLOT(setDisabled(bool)));

	connect(m_option_paragraphs, SIGNAL(toggled(bool)), m_page_characters, SLOT(setDisabled(bool)));
	connect(m_option_paragraphs, SIGNAL(toggled(bool)), m_page_paragraphs, SLOT(setEnabled(bool)));
	connect(m_option_paragraphs, SIGNAL(toggled(bool)), m_page_words, SLOT(setDisabled(bool)));

	connect(m_option_words, SIGNAL(toggled(bool)), m_page_characters, SLOT(setDisabled(bool)));
	connect(m_option_words, SIGNAL(toggled(bool)), m_page_paragraphs, SLOT(setDisabled(bool)));
	connect(m_option_words, SIGNAL(toggled(bool)), m_page_words, SLOT(setEnabled(bool)));

	QGridLayout* page_layout = new QGridLayout(page_group);
	page_layout->setColumnStretch(2, 1);
	page_layout->addWidget(m_option_characters, 0, 0);
	page_layout->addWidget(m_page_characters, 0, 1);
	page_layout->addWidget(m_option_paragraphs, 1, 0);
	page_layout->addWidget(m_page_paragraphs, 1, 1);
	page_layout->addWidget(m_option_words, 2, 0);
	page_layout->addWidget(m_page_words, 2, 1);

	// Lay out statistics options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addLayout(counts_layout);
	layout->addWidget(wordcount_group);
	layout->addWidget(page_group);
	layout->addStretch();

	return makeScrollable(tab);
}

//-----------------------------------------------------------------------------

QWidget* PreferencesDialog::initSpellingTab()
{
	QWidget* tab = new QWidget(this);

	// Create spelling options
	QWidget* general_group = new QWidget(tab);

	m_highlight_misspelled = new QCheckBox(tr("Check spelling as you type"), general_group);
	m_ignore_uppercase = new QCheckBox(tr("Ignore words in UPPERCASE"), general_group);
	m_ignore_numbers = new QCheckBox(tr("Ignore words with numbers"), general_group);
#ifdef Q_OS_MAC
	m_ignore_uppercase->hide();
	m_ignore_numbers->hide();
#endif

	QVBoxLayout* general_group_layout = new QVBoxLayout(general_group);
	general_group_layout->setMargin(0);
	general_group_layout->addWidget(m_highlight_misspelled);
	general_group_layout->addWidget(m_ignore_uppercase);
	general_group_layout->addWidget(m_ignore_numbers);

	// Create language selection
	QGroupBox* languages_group = new QGroupBox(tr("Language"), tab);

	m_languages = new QComboBox(languages_group);
	connect(m_languages, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedLanguageChanged(int)));

	m_add_language_button = new QPushButton(tr("Add"), languages_group);
	m_add_language_button->setAutoDefault(false);
	connect(m_add_language_button, SIGNAL(clicked()), this, SLOT(addLanguage()));
	m_remove_language_button = new QPushButton(tr("Remove"), languages_group);
	m_remove_language_button->setAutoDefault(false);
	connect(m_remove_language_button, SIGNAL(clicked()), this, SLOT(removeLanguage()));

	QStringList languages = DictionaryManager::instance().availableDictionaries();
	for (const QString& language : languages) {
		m_languages->addItem(LocaleDialog::languageName(language), language);
	}
	m_languages->model()->sort(0);

	// Lay out language selection
	QHBoxLayout* languages_layout = new QHBoxLayout(languages_group);
	languages_layout->addWidget(m_languages, 1);
	languages_layout->addWidget(m_add_language_button);
	languages_layout->addWidget(m_remove_language_button);

	// Read personal dictionary
	QGroupBox* personal_dictionary_group = new QGroupBox(tr("Personal Dictionary"), tab);

	m_word = new QLineEdit(personal_dictionary_group);
	connect(m_word, SIGNAL(textChanged(QString)), this, SLOT(wordEdited()));

	m_add_word_button = new QPushButton(tr("Add"), personal_dictionary_group);
	m_add_word_button->setAutoDefault(false);
	m_add_word_button->setDisabled(true);
	connect(m_add_word_button, SIGNAL(clicked()), this, SLOT(addWord()));

	m_personal_dictionary = new QListWidget(personal_dictionary_group);
	QStringList words = DictionaryManager::instance().personal();
	for (const QString& word : words) {
		m_personal_dictionary->addItem(word);
	}
	connect(m_personal_dictionary, SIGNAL(itemSelectionChanged()), this, SLOT(selectedWordChanged()));

	m_remove_word_button = new QPushButton(tr("Remove"), personal_dictionary_group);
	m_remove_word_button->setAutoDefault(false);
	m_remove_word_button->setDisabled(true);
	connect(m_remove_word_button, SIGNAL(clicked()), this, SLOT(removeWord()));

	// Lay out personal dictionary group
	QGridLayout* personal_dictionary_layout = new QGridLayout(personal_dictionary_group);
	personal_dictionary_layout->addWidget(m_word, 0, 0);
	personal_dictionary_layout->addWidget(m_add_word_button, 0, 1);
	personal_dictionary_layout->addWidget(m_personal_dictionary, 1, 0);
	personal_dictionary_layout->addWidget(m_remove_word_button, 1, 1, Qt::AlignTop);

	// Lay out spelling options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addWidget(general_group);
	layout->addWidget(languages_group);
	layout->addWidget(personal_dictionary_group);

	return makeScrollable(tab);
}

//-----------------------------------------------------------------------------

QWidget* PreferencesDialog::initToolbarTab()
{
	QWidget* tab = new QWidget(this);

	// Create style options
	QGroupBox* style_group = new QGroupBox(tr("Style"), tab);

	m_toolbar_style = new QComboBox(style_group);
	m_toolbar_style->addItem(tr("Icons Only"), Qt::ToolButtonIconOnly);
	m_toolbar_style->addItem(tr("Text Only"), Qt::ToolButtonTextOnly);
	m_toolbar_style->addItem(tr("Text Alongside Icons"), Qt::ToolButtonTextBesideIcon);
	m_toolbar_style->addItem(tr("Text Under Icons"), Qt::ToolButtonTextUnderIcon);

	// Lay out style options
	QFormLayout* style_layout = new QFormLayout(style_group);
	style_layout->addRow(tr("Text Position:"), m_toolbar_style);

	// Create action options
	QGroupBox* actions_group = new QGroupBox(tr("Actions"), tab);

	m_toolbar_actions = new QListWidget(actions_group);
	m_toolbar_actions->setDragDropMode(QAbstractItemView::InternalMove);
	QList<QAction*> actions = parentWidget()->window()->actions();
	for (QAction* action : actions) {
		if (action->data().isNull()) {
			continue;
		}
		QListWidgetItem* item = new QListWidgetItem(action->icon(), action->iconText(), m_toolbar_actions);
		item->setData(Qt::UserRole, action->data());
		item->setCheckState(Qt::Unchecked);
	}
	m_toolbar_actions->sortItems();
	connect(m_toolbar_actions, SIGNAL(currentRowChanged(int)), this, SLOT(currentActionChanged(int)));

	m_move_up_button = new QPushButton(tr("Move Up"), actions_group);
	connect(m_move_up_button, SIGNAL(clicked()), this, SLOT(moveActionUp()));
	m_move_down_button = new QPushButton(tr("Move Down"), actions_group);
	connect(m_move_down_button, SIGNAL(clicked()), this, SLOT(moveActionDown()));
	QPushButton* add_separator_button = new QPushButton(tr("Add Separator"), actions_group);
	connect(add_separator_button, SIGNAL(clicked()), this, SLOT(addSeparatorAction()));

	// Lay out action options
	QGridLayout* actions_layout = new QGridLayout(actions_group);
	actions_layout->setRowStretch(0, 1);
	actions_layout->setRowStretch(4, 1);
	actions_layout->addWidget(m_toolbar_actions, 0, 0, 5, 1);
	actions_layout->addWidget(m_move_up_button, 1, 1);
	actions_layout->addWidget(m_move_down_button, 2, 1);
	actions_layout->addWidget(add_separator_button, 3, 1);

	// Lay out toolbar tab
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addWidget(style_group);
	layout->addWidget(actions_group);

	return makeScrollable(tab);
}

//-----------------------------------------------------------------------------

QWidget* PreferencesDialog::initShortcutsTab()
{
	QWidget* tab = new QWidget(this);

	// Create shortcuts view
	m_shortcuts = new QTreeWidget(tab);
	m_shortcuts->setIconSize(QSize(16,16));
	m_shortcuts->setDragDropMode(QAbstractItemView::NoDragDrop);
	m_shortcuts->setItemsExpandable(false);
	m_shortcuts->setRootIsDecorated(false);
	m_shortcuts->setColumnCount(3);
	m_shortcuts->setColumnHidden(2, true);
	m_shortcuts->setHeaderLabels(QStringList() << tr("Command") << tr("Shortcut") << tr("Action"));
	m_shortcuts->header()->setSectionsClickable(false);
	m_shortcuts->header()->setSectionsMovable(false);
	m_shortcuts->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	connect(m_shortcuts, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(shortcutDoubleClicked()));

	// List shortcuts
	QPixmap empty_icon(m_shortcuts->iconSize());
	empty_icon.fill(Qt::transparent);
	QList<QString> actions = ActionManager::instance()->actions();
	for (const QString& name : actions) {
		QAction* action = ActionManager::instance()->action(name);
		QIcon icon = action->icon();
		if (icon.isNull()) {
			icon = empty_icon;
		}
		QString text = action->statusTip();
		if (text.isEmpty()) {
			text = action->text();
		}
		text.replace("&", "");
		QStringList strings = QStringList() << text << action->shortcut().toString(QKeySequence::NativeText) << name;
		QTreeWidgetItem* item = new QTreeWidgetItem(m_shortcuts, strings);
		item->setIcon(0, icon);
	}
	m_shortcuts->sortByColumn(0, Qt::AscendingOrder);
	connect(m_shortcuts, SIGNAL(itemSelectionChanged()), this, SLOT(selectedShortcutChanged()));

	// Create editor
	m_shortcut_edit = new ShortcutEdit(this);
	connect(m_shortcut_edit, SIGNAL(changed()), this, SLOT(shortcutChanged()));

	// Lay out shortcut tab
	QGridLayout* layout = new QGridLayout(tab);
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(0, 1);
	layout->addWidget(m_shortcuts, 0, 0, 1, 2);
	layout->addWidget(new QLabel(ShortcutEdit::tr("Shortcut:"), tab), 1, 0);
	layout->addWidget(m_shortcut_edit, 1, 1);

	m_shortcuts->setCurrentItem(m_shortcuts->topLevelItem(0));
	highlightShortcutConflicts();

	return makeScrollable(tab);
}

//-----------------------------------------------------------------------------
