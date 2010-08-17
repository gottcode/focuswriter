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

#include "preferences_dialog.h"

#include "dictionary.h"
#include "preferences.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <zip.h>

/*****************************************************************************/

namespace {
	QString languageName(const QString& language) {
		QLocale locale(language.left(5));
		QString name = QLocale::languageToString(locale.language());
		if (locale.country() != QLocale::AnyCountry) {
			name += " (" + QLocale::countryToString(locale.country()) + ")";
		}
		return name;
	}
}

/*****************************************************************************/

PreferencesDialog::PreferencesDialog(Preferences& preferences, QWidget* parent)
: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
  m_preferences(preferences) {
	setWindowTitle(tr("Preferences"));
	m_dictionary = new Dictionary(this);

	QTabWidget* tabs = new QTabWidget(this);
	tabs->addTab(initGeneralTab(), tr("General"));
	tabs->addTab(initStatisticsTab(), tr("Statistics"));
	tabs->addTab(initToolbarTab(), tr("Toolbar"));
	tabs->addTab(initSpellingTab(), tr("Spell Checking"));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(tabs);
	layout->addWidget(buttons);

	// Load settings
	switch (m_preferences.goalType()) {
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
	m_time->setValue(m_preferences.goalMinutes());
	m_wordcount->setValue(m_preferences.goalWords());

	m_show_characters->setChecked(m_preferences.showCharacters());
	m_show_pages->setChecked(m_preferences.showPages());
	m_show_paragraphs->setChecked(m_preferences.showParagraphs());
	m_show_words->setChecked(m_preferences.showWords());

	switch (m_preferences.pageType()) {
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
	m_page_characters->setValue(m_preferences.pageCharacters());
	m_page_paragraphs->setValue(m_preferences.pageParagraphs());
	m_page_words->setValue(m_preferences.pageWords());

	if (m_preferences.accurateWordcount()) {
		m_option_accurate_wordcount->setChecked(true);
	} else {
		m_option_estimate_wordcount->setChecked(true);
	}

	m_always_center->setChecked(m_preferences.alwaysCenter());
	m_block_cursor->setChecked(m_preferences.blockCursor());
	m_rich_text->setChecked(m_preferences.richText());
	m_smooth_fonts->setChecked(m_preferences.smoothFonts());

	m_auto_save->setChecked(m_preferences.autoSave());
	m_save_positions->setChecked(m_preferences.savePositions());

	int style = m_toolbar_style->findData(m_preferences.toolbarStyle());
	if (style == -1) {
		style = m_toolbar_style->findData(Qt::ToolButtonTextUnderIcon);
	}
	m_toolbar_style->setCurrentIndex(style);
	QStringList actions = m_preferences.toolbarActions();
	int pos = 0;
	foreach (const QString& action, actions) {
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

	m_highlight_misspelled->setChecked(m_preferences.highlightMisspelled());
	m_ignore_numbers->setChecked(m_preferences.ignoredWordsWithNumbers());
	m_ignore_uppercase->setChecked(m_preferences.ignoredUppercaseWords());
	int index = m_languages->findData(m_preferences.language());
	if (index != -1) {
		m_languages->setCurrentIndex(index);
	}
}

/*****************************************************************************/

void PreferencesDialog::accept() {
	// Save settings
	if (m_option_time->isChecked()) {
		m_preferences.setGoalType(1);
	} else if (m_option_wordcount->isChecked()) {
		m_preferences.setGoalType(2);
	} else {
		m_preferences.setGoalType(0);
	}
	m_preferences.setGoalMinutes(m_time->value());
	m_preferences.setGoalWords(m_wordcount->value());

	m_preferences.setShowCharacters(m_show_characters->isChecked());
	m_preferences.setShowPages(m_show_pages->isChecked());
	m_preferences.setShowParagraphs(m_show_paragraphs->isChecked());
	m_preferences.setShowWords(m_show_words->isChecked());

	if (m_option_paragraphs->isChecked()) {
		m_preferences.setPageType(1);
	} else if (m_option_words->isChecked()) {
		m_preferences.setPageType(2);
	} else {
		m_preferences.setPageType(0);
	}
	m_preferences.setPageCharacters(m_page_characters->value());
	m_preferences.setPageParagraphs(m_page_paragraphs->value());
	m_preferences.setPageWords(m_page_words->value());

	m_preferences.setAccurateWordcount(m_option_accurate_wordcount->isChecked());

	m_preferences.setAlwaysCenter(m_always_center->isChecked());
	m_preferences.setBlockCursor(m_block_cursor->isChecked());
	m_preferences.setRichText(m_rich_text->isChecked());
	m_preferences.setSmoothFonts(m_smooth_fonts->isChecked());

	m_preferences.setAutoSave(m_auto_save->isChecked());
	m_preferences.setSavePositions(m_save_positions->isChecked());

	m_preferences.setToolbarStyle(m_toolbar_style->itemData(m_toolbar_style->currentIndex()).toInt());
	QStringList actions;
	int count = m_toolbar_actions->count();
	for (int i = 0; i < count; ++i) {
		QListWidgetItem* item = m_toolbar_actions->item(i);
		QString action = (item->checkState() == Qt::Unchecked ? "^" : "") + item->data(Qt::UserRole).toString();
		if (action != "^|") {
			actions.append(action);
		}
	}
	m_preferences.setToolbarActions(actions);

	// Uninstall languages
	foreach (const QString& language, m_uninstalled) {
		QFile::remove("dict:" + language + ".aff");
		QFile::remove("dict:" + language + ".dic");
	}

	// Install languages
	QString path = Dictionary::path() + "/install/";
	QDir dir(path);
	QStringList files = dir.entryList(QDir::Files);
	foreach (const QString& file, files) {
		QFile::remove(path + "/../" + file);
		QString new_file = file;
		new_file.replace(QChar('-'), QChar('_'));
		QFile::rename(path + file, path + "/../" + new_file);
	}
	dir.cdUp();
	dir.rmdir("install");

	// Set dictionary
	m_preferences.setHighlightMisspelled(m_highlight_misspelled->isChecked());
	m_preferences.setIgnoreWordsWithNumbers(m_ignore_numbers->isChecked());
	m_preferences.setIgnoreUppercaseWords(m_ignore_uppercase->isChecked());
	if (m_languages->count()) {
		m_preferences.setLanguage(m_languages->itemData(m_languages->currentIndex()).toString());
	} else {
		m_preferences.setLanguage(QString());
	}
	m_dictionary->setIgnoreNumbers(m_preferences.ignoredWordsWithNumbers());
	m_dictionary->setIgnoreUppercase(m_preferences.ignoredUppercaseWords());
	m_dictionary->setLanguage(m_preferences.language());

	// Save personal dictionary
	QStringList words;
	for (int i = 0; i < m_personal_dictionary->count(); ++i) {
		words.append(m_personal_dictionary->item(i)->text());
	}
	m_dictionary->setPersonal(words);

	QDialog::accept();
}

/*****************************************************************************/

void PreferencesDialog::reject() {
	QDir dir(Dictionary::path() + "/install/");
	if (dir.exists()) {
		QStringList files = dir.entryList(QDir::Files);
		foreach (const QString& file, files) {
			QFile::remove(dir.filePath(file));
		}
		dir.cdUp();
		dir.rmdir("install");
	}
	QDialog::reject();
}

/*****************************************************************************/

void PreferencesDialog::moveActionUp() {
	int from = m_toolbar_actions->currentRow();
	int to = from - 1;
	if (from > 0) {
		m_toolbar_actions->insertItem(to, m_toolbar_actions->takeItem(from));
		m_toolbar_actions->setCurrentRow(to);
	}
}

/*****************************************************************************/

void PreferencesDialog::moveActionDown() {
	int from = m_toolbar_actions->currentRow();
	int to = from + 1;
	if (to < m_toolbar_actions->count()) {
		m_toolbar_actions->insertItem(to, m_toolbar_actions->takeItem(from));
		m_toolbar_actions->setCurrentRow(to);
	}
}

/*****************************************************************************/

void PreferencesDialog::addSeparatorAction() {
	QListWidgetItem* item = new QListWidgetItem(QString(20, QChar('-')));
	item->setCheckState(Qt::Checked);
	item->setData(Qt::UserRole, "|");
	m_toolbar_actions->insertItem(m_toolbar_actions->currentRow(), item);
}

/*****************************************************************************/

void PreferencesDialog::currentActionChanged(int action) {
	if (action != -1) {
		m_move_up_button->setEnabled(action > 0);
		m_move_down_button->setEnabled((action + 1) < m_toolbar_actions->count());
	}
}

/*****************************************************************************/

void PreferencesDialog::addLanguage() {
	QString path = QFileDialog::getOpenFileName(this, tr("Select Dictionary"), QDir::homePath());
	if (path.isEmpty()) {
		return;
	}

	// File lists
	QHash<QString, int> aff_files;
	QHash<QString, int> dic_files;
	QHash<QString, int> files;
	QStringList dictionaries;

	// Open archive
	zip* archive = zip_open(path.toUtf8().data(), 0, 0);
	if (!archive) {
		QMessageBox::warning(this, tr("Sorry"), tr("Unable to open archive."));
		return;
	}

	try {
		// List files
		int count = zip_get_num_files(archive);
		if (count == -1) {
			throw tr("Unable to read archive metadata.");
		}
		for (int i = 0; i < count; ++i) {
			QString name = QString::fromUtf8(zip_get_name(archive, i, 0));
			if (name.endsWith(".aff")) {
				aff_files[name] = i;
			} else if (name.endsWith(".dic")) {
				dic_files[name] = i;
			}
		}

		// Find dictionary files
		foreach (const QString& dic, dic_files.keys()) {
			QString aff = dic;
			aff.replace(".dic", ".aff");
			if (aff_files.contains(aff)) {
				files[dic] = dic_files[dic];
				files[aff] = aff_files[aff];
				QString dictionary = dic.section('/', -1);
				dictionary.chop(4);
				dictionaries += dictionary;
			}
		}

		// Extract files
		QDir dir(Dictionary::path());
		dir.mkdir("install");
		QString install = dir.absoluteFilePath("install") + "/";
		QHashIterator<QString, int> i(files);
		while (i.hasNext()) {
			i.next();

			QFile file(install + i.key().section('/', -1));
			if (file.open(QIODevice::WriteOnly)) {
				zip_file* zfile = zip_fopen_index(archive, i.value(), 0);
				if (zfile == 0) {
					throw tr("Unable to open file '%1'.").arg(i.key());
				}

				char buffer[8192];
				int len;
				while ((len = zip_fread(zfile, &buffer, sizeof(buffer))) > 0) {
					file.write(buffer, len);
				}
				file.close();

				if (zip_fclose(zfile) != 0) {
					throw tr("Unable to close file '%1'.").arg(i.key());
				}
			}
		}

		// Add to language selection
		QString dictionary_path = Dictionary::path() + "/install/";
		QString dictionary_new_path = Dictionary::path() + "/";
		foreach (const QString& dictionary, dictionaries) {
			QString language = dictionary;
			language.replace(QChar('-'), QChar('_'));
			QString name = languageName(language);

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
	}

	catch (QString error) {
		QMessageBox::warning(this, tr("Sorry"), error);
	}

	// Close archive
	zip_close(archive);
}

/*****************************************************************************/

void PreferencesDialog::removeLanguage() {
	int index = m_languages->currentIndex();
	if (index == -1) {
		return;
	}
	if (QMessageBox::question(this, tr("Question"), tr("Remove current dictionary?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		m_uninstalled.append(m_languages->itemData(index).toString());
		m_languages->removeItem(index);
	}
}

/*****************************************************************************/

void PreferencesDialog::selectedLanguageChanged(int index) {
	if (index != -1) {
		QFileInfo info("dict:" + m_languages->itemData(index).toString() + ".dic");
		m_remove_language_button->setEnabled(info.canonicalFilePath().startsWith(Dictionary::path()));
	}
}

/*****************************************************************************/

void PreferencesDialog::addWord() {
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

/*****************************************************************************/

void PreferencesDialog::removeWord() {
	delete m_personal_dictionary->selectedItems().first();
	m_personal_dictionary->clearSelection();
}

/*****************************************************************************/

void PreferencesDialog::selectedWordChanged() {
	m_remove_word_button->setDisabled(m_personal_dictionary->selectedItems().isEmpty());
}

/*****************************************************************************/

void PreferencesDialog::wordEdited() {
	QString word = m_word->text();
	m_add_word_button->setEnabled(!word.isEmpty() && m_personal_dictionary->findItems(word, Qt::MatchExactly).isEmpty());
}

/*****************************************************************************/

QWidget* PreferencesDialog::initGeneralTab() {
	QWidget* tab = new QWidget(this);

	// Create goal options
	QGroupBox* goals_group = new QGroupBox(tr("Daily Goal"), tab);

	m_option_none = new QRadioButton(tr("None"), goals_group);

	m_option_time = new QRadioButton(tr("Minutes:"), goals_group);

	m_time = new QSpinBox(goals_group);
	m_time->setRange(5, 1440);
	m_time->setSingleStep(5);

	QHBoxLayout* time_layout = new QHBoxLayout;
	time_layout->addWidget(m_option_time);
	time_layout->addWidget(m_time);
	time_layout->addStretch();

	m_option_wordcount = new QRadioButton(tr("Words:"), goals_group);

	m_wordcount = new QSpinBox(goals_group);
	m_wordcount->setRange(100, 100000);
	m_wordcount->setSingleStep(100);

	QHBoxLayout* wordcount_layout = new QHBoxLayout;
	wordcount_layout->addWidget(m_option_wordcount);
	wordcount_layout->addWidget(m_wordcount);
	wordcount_layout->addStretch();

	QVBoxLayout* goals_layout = new QVBoxLayout(goals_group);
	goals_layout->addWidget(m_option_none);
	goals_layout->addLayout(time_layout);
	goals_layout->addLayout(wordcount_layout);

	// Create edit options
	QGroupBox* edit_group = new QGroupBox(tr("Editing"), tab);

	m_always_center = new QCheckBox(tr("Always vertically center"), edit_group);
	m_block_cursor = new QCheckBox(tr("Block insertion cursor"), edit_group);
	m_rich_text = new QCheckBox(tr("Default to rich text"), edit_group);
	m_smooth_fonts = new QCheckBox(tr("Smooth fonts"), edit_group);

	QVBoxLayout* edit_layout = new QVBoxLayout(edit_group);
	edit_layout->addWidget(m_always_center);
	edit_layout->addWidget(m_block_cursor);
	edit_layout->addWidget(m_rich_text);
	edit_layout->addWidget(m_smooth_fonts);

	// Create save options
	QGroupBox* save_group = new QGroupBox(tr("Saving"), tab);

	m_auto_save = new QCheckBox(tr("Automatically save changes"), save_group);
	m_save_positions = new QCheckBox(tr("Remember cursor position"), save_group);

	QVBoxLayout* save_layout = new QVBoxLayout(save_group);
	save_layout->addWidget(m_auto_save);
	save_layout->addWidget(m_save_positions);

	// Lay out general options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addWidget(goals_group);
	layout->addWidget(edit_group);
	layout->addWidget(save_group);
	layout->addStretch();

	return tab;
}

/*****************************************************************************/

QWidget* PreferencesDialog::initStatisticsTab() {
	QWidget* tab = new QWidget(this);

	// Create statistics options
	QGroupBox* counts_group = new QGroupBox(tr("Contents"), tab);

	m_show_characters = new QCheckBox(tr("Character count"), counts_group);
	m_show_pages = new QCheckBox(tr("Page count"), counts_group);
	m_show_paragraphs = new QCheckBox(tr("Paragraph count"), counts_group);
	m_show_words = new QCheckBox(tr("Word count"), counts_group);

	QVBoxLayout* counts_layout = new QVBoxLayout(counts_group);
	counts_layout->addWidget(m_show_words);
	counts_layout->addWidget(m_show_pages);
	counts_layout->addWidget(m_show_paragraphs);
	counts_layout->addWidget(m_show_characters);

	// Create page algorithm options
	QGroupBox* page_group = new QGroupBox(tr("Page Size"), tab);

	m_option_characters = new QRadioButton(tr("Characters:"), page_group);
	m_page_characters = new QSpinBox(page_group);
	m_page_characters->setRange(500, 10000);
	m_page_characters->setSingleStep(250);
	QHBoxLayout* characters_layout = new QHBoxLayout;
	characters_layout->addWidget(m_option_characters);
	characters_layout->addWidget(m_page_characters);
	characters_layout->addStretch();

	m_option_paragraphs = new QRadioButton(tr("Paragraphs:"), page_group);
	m_page_paragraphs = new QSpinBox(page_group);
	m_page_paragraphs->setRange(1, 100);
	m_page_paragraphs->setSingleStep(1);
	QHBoxLayout* paragraphs_layout = new QHBoxLayout;
	paragraphs_layout->addWidget(m_option_paragraphs);
	paragraphs_layout->addWidget(m_page_paragraphs);
	paragraphs_layout->addStretch();

	m_option_words = new QRadioButton(tr("Words:"), page_group);
	m_page_words = new QSpinBox(page_group);
	m_page_words->setRange(100, 2000);
	m_page_words->setSingleStep(50);
	QHBoxLayout* words_layout = new QHBoxLayout;
	words_layout->addWidget(m_option_words);
	words_layout->addWidget(m_page_words);
	words_layout->addStretch();

	QVBoxLayout* page_layout = new QVBoxLayout(page_group);
	page_layout->addLayout(characters_layout);
	page_layout->addLayout(paragraphs_layout);
	page_layout->addLayout(words_layout);

	// Create wordcount options
	QGroupBox* wordcount_group = new QGroupBox(tr("Word Count Algorithm"), this);

	m_option_accurate_wordcount = new QRadioButton(tr("Detect word boundaries"), wordcount_group);
	m_option_estimate_wordcount = new QRadioButton(tr("Divide character count by six"), wordcount_group);

	QVBoxLayout* wordcount_layout = new QVBoxLayout(wordcount_group);
	wordcount_layout->addWidget(m_option_accurate_wordcount);
	wordcount_layout->addWidget(m_option_estimate_wordcount);

	// Lay out statistics options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addWidget(counts_group);
	layout->addWidget(page_group);
	layout->addWidget(wordcount_group);
	layout->addStretch();

	return tab;
}

/*****************************************************************************/

QWidget* PreferencesDialog::initToolbarTab() {
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
	foreach (QAction* action, actions) {
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

	return tab;
}

/*****************************************************************************/

QWidget* PreferencesDialog::initSpellingTab() {
	QWidget* tab = new QWidget(this);

	// Create spelling options
	QWidget* general_group = new QWidget(tab);

	m_highlight_misspelled = new QCheckBox(tr("Check spelling as you type"), general_group);
	m_ignore_uppercase = new QCheckBox(tr("Ignore words in UPPERCASE"), general_group);
	m_ignore_numbers = new QCheckBox(tr("Ignore words with numbers"), general_group);

	QVBoxLayout* general_group_layout = new QVBoxLayout(general_group);
	general_group_layout->setMargin(0);
	general_group_layout->addWidget(m_highlight_misspelled);
	general_group_layout->addWidget(m_ignore_uppercase);
	general_group_layout->addWidget(m_ignore_numbers);

	// Create language selection
	QGroupBox* languages_group = new QGroupBox(tr("Language"), tab);

	m_add_language_button = new QPushButton(tr("Add"), languages_group);
	m_add_language_button->setAutoDefault(false);
	connect(m_add_language_button, SIGNAL(clicked()), this, SLOT(addLanguage()));
	m_remove_language_button = new QPushButton(tr("Remove"), languages_group);
	m_remove_language_button->setAutoDefault(false);
	connect(m_remove_language_button, SIGNAL(clicked()), this, SLOT(removeLanguage()));

	m_languages = new QComboBox(languages_group);
	connect(m_languages, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedLanguageChanged(int)));

	QStringList languages = m_dictionary->availableLanguages();
	foreach (const QString& language, languages) {
		m_languages->addItem(languageName(language), language);
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
	connect(m_word, SIGNAL(textChanged(const QString&)), this, SLOT(wordEdited()));

	m_personal_dictionary = new QListWidget(personal_dictionary_group);
	QStringList words = m_dictionary->personal();
	foreach (const QString& word, words) {
		m_personal_dictionary->addItem(word);
	}
	connect(m_personal_dictionary, SIGNAL(itemSelectionChanged()), this, SLOT(selectedWordChanged()));

	m_add_word_button = new QPushButton(tr("Add"), personal_dictionary_group);
	m_add_word_button->setAutoDefault(false);
	m_add_word_button->setDisabled(true);
	connect(m_add_word_button, SIGNAL(clicked()), this, SLOT(addWord()));
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

	return tab;
}

/*****************************************************************************/
