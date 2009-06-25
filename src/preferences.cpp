/***********************************************************************
 *
 * Copyright (C) 2008-2009 Graeme Gott <graeme@gottcode.org>
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
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

/*****************************************************************************/

namespace {
	QString languageName(const QString& language) {
		QLocale locale(language);
		QString name = QLocale::languageToString(locale.language());
		if ((locale.country() != QLocale::AnyCountry) && (language.length() == 5)) {
			name += " (" + QLocale::countryToString(locale.country()) + ")";
		}
		return name;
	}
}

/*****************************************************************************/

Preferences::Preferences(QWidget* parent)
: QDialog(parent) {
	setWindowTitle(tr("Preferences"));
	m_dictionary = new Dictionary(this);

	QTabWidget* tabs = new QTabWidget(this);
	tabs->addTab(initGeneralTab(), tr("General"));
	tabs->addTab(initSpellingTab(), tr("Spell Checking"));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(tabs);
	layout->addWidget(buttons);

	// Load settings
	QSettings settings;
	switch (settings.value("Goal/Type", 1).toInt()) {
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
	m_time->setValue(settings.value("Goal/Minutes", 30).toInt());
	m_wordcount->setValue(settings.value("Goal/Words", 1000).toInt());
	m_show_characters->setChecked(settings.value("Stats/ShowCharacters", true).toBool());
	m_show_pages->setChecked(settings.value("Stats/ShowPages", true).toBool());
	m_show_paragraphs->setChecked(settings.value("Stats/ShowParagraphs", true).toBool());
	m_show_words->setChecked(settings.value("Stats/ShowWords", true).toBool());
	m_always_center->setChecked(settings.value("Edit/AlwaysCenter", false).toBool());
	m_location->setText(settings.value("Save/Location", QDir::currentPath()).toString());
	m_auto_save->setChecked(settings.value("Save/Auto", true).toBool());
	m_auto_append->setChecked(settings.value("Save/Append", true).toBool());
	m_highlight_misspelled->setChecked(settings.value("Spelling/HighlightMisspelled", true).toBool());
	m_ignore_numbers->setChecked(settings.value("Spelling/IgnoreNumbers", true).toBool());
	m_ignore_uppercase->setChecked(settings.value("Spelling/IgnoreUppercase", true).toBool());
	int index = m_languages->findData(settings.value("Spelling/Language", QLocale::system().name()).toString());
	if (index != -1) {
		m_languages->setCurrentIndex(index);
	}
	m_dictionary->setLanguage(language());
	m_dictionary->setIgnoreNumbers(ignoredWordsWithNumbers());
	m_dictionary->setIgnoreUppercase(ignoredUppercaseWords());
}

/*****************************************************************************/

int Preferences::goalType() const {
	if (m_option_time->isChecked()) {
		return 1;
	} else if (m_option_wordcount->isChecked()) {
		return 2;
	} else {
		return 0;
	}
}

/*****************************************************************************/

int Preferences::goalMinutes() const {
	return m_time->value();
}

/*****************************************************************************/

int Preferences::goalWords() const {
	return m_wordcount->value();
}

/*****************************************************************************/

bool Preferences::showCharacters() const {
	return m_show_characters->isChecked();
}

/*****************************************************************************/

bool Preferences::showPages() const {
	return m_show_pages->isChecked();
}

/*****************************************************************************/

bool Preferences::showParagraphs() const {
	return m_show_paragraphs->isChecked();
}

/*****************************************************************************/

bool Preferences::showWords() const {
	return m_show_words->isChecked();
}

/*****************************************************************************/

bool Preferences::alwaysCenter() const {
	return m_always_center->isChecked();
}

/*****************************************************************************/

QString Preferences::saveLocation() const {
	return m_location->text();
}

/*****************************************************************************/

bool Preferences::autoSave() const {
	return m_auto_save->isChecked();
}

/*****************************************************************************/

bool Preferences::autoAppend() const {
	return m_auto_append->isChecked();
}

/*****************************************************************************/

bool Preferences::highlightMisspelled() const {
	return m_highlight_misspelled->isChecked();
}

/*****************************************************************************/

bool Preferences::ignoredWordsWithNumbers() const {
	return m_ignore_numbers->isChecked();
}

/*****************************************************************************/

bool Preferences::ignoredUppercaseWords() const {
	return m_ignore_uppercase->isChecked();
}

/*****************************************************************************/

QString Preferences::language() const {
	if (m_languages->count()) {
		return m_languages->itemData(m_languages->currentIndex()).toString();
	} else {
		return QString();
	}
}

/*****************************************************************************/

void Preferences::accept() {
	QSettings settings;
	settings.setValue("Goal/Type", goalType());
	settings.setValue("Goal/Minutes", goalMinutes());
	settings.setValue("Goal/Words", goalWords());
	settings.setValue("Stats/ShowCharacters", showCharacters());
	settings.setValue("Stats/ShowPages", showPages());
	settings.setValue("Stats/ShowParagraphs", showParagraphs());
	settings.setValue("Stats/ShowWords", showWords());
	settings.setValue("Edit/AlwaysCenter", alwaysCenter());
	settings.setValue("Save/Auto", autoSave());
	settings.setValue("Save/Append", autoAppend());
	settings.setValue("Save/Location", m_location->text());
	QDir::setCurrent(m_location->text());

	settings.setValue("Spelling/HighlightMisspelled", highlightMisspelled());
	settings.setValue("Spelling/IgnoreNumbers", ignoredWordsWithNumbers());
	settings.setValue("Spelling/IgnoreUppercase", ignoredUppercaseWords());
	settings.setValue("Spelling/Language", language());
	m_dictionary->setIgnoreNumbers(ignoredWordsWithNumbers());
	m_dictionary->setIgnoreUppercase(ignoredUppercaseWords());
	m_dictionary->setLanguage(language());

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

	// Save personal dictionary
	QStringList words;
	for (int i = 0; i < m_personal_dictionary->count(); ++i) {
		words.append(m_personal_dictionary->item(i)->text());
	}
	m_dictionary->setPersonal(words);

	QDialog::accept();
}

/*****************************************************************************/

void Preferences::reject() {
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

void Preferences::changeSaveLocation() {
	QString location = QFileDialog::getExistingDirectory(this, tr("Find Directory"), m_location->text());
	if (!location.isEmpty()) {
		m_location->setText(location);
	}
}

/*****************************************************************************/

void Preferences::addLanguage() {
	QString path = QFileDialog::getOpenFileName(this, tr("Select Dictionary"), QDir::homePath());
	if (path.isEmpty()) {
		return;
	}

	// File lists
	QStringList files;
	QStringList aff_files;
	QStringList dic_files;
	QStringList dictionaries;

	// List files in archive
	QProcess list_files;
	list_files.setWorkingDirectory(Dictionary::path());
	list_files.start(QString("unzip -qq -l %1").arg(path));
	list_files.waitForFinished(-1);
	if (list_files.exitCode() != 0) {
		QMessageBox::warning(this, tr("Error"), tr("Unable to list files in archive."));
		return;
	}
	QStringList lines = QString(list_files.readAllStandardOutput()).split(QChar('\n'), QString::SkipEmptyParts);
	foreach (const QString& line, lines) {
		// Fetch file name
		int index = line.lastIndexOf(QChar(' '));
		if (index == -1) {
			continue;
		}
		QString file = line.mid(index + 1);

		// Determine file type
		if (file.endsWith(".aff")) {
			aff_files.append(file);
		} else if (file.endsWith(".dic")) {
			dic_files.append(file);
		} else {
			continue;
		}
	}

	// Find dictionary files
	foreach (const QString& dic, dic_files) {
		QString aff = dic;
		aff.replace(".dic", ".aff");
		if (aff_files.contains(aff)) {
			files.append(dic);
			files.append(aff);
			int index = dic.lastIndexOf(QChar('/')) + 1;
			dictionaries.append(dic.mid(index, dic.length() - index - 4));
		}
	}

	// Extract files
	if (files.isEmpty()) {
		return;
	}
	QProcess unzip;
	unzip.setWorkingDirectory(Dictionary::path());
	unzip.start(QString("unzip -qq -o -j %1 %2 -d install").arg(path).arg(files.join(" ")));
	unzip.waitForFinished(-1);
	if (unzip.exitCode() != 0) {
		QMessageBox::warning(this, tr("Error"), tr("Unable to extract dictionary files from archive."));
		return;
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

		if (QFile::exists(new_aff_file) || QFile::exists(new_dic_file)) {
			if (QMessageBox::question(this, tr("Question"), tr("The dictionary \"%1\" already exists. Do you want to replace it?").arg(name), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
				QFile::remove(aff_file);
				QFile::remove(dic_file);
			}
			continue;
		}

		m_languages->addItem(name, language);
	}
	m_languages->model()->sort(0);
}

/*****************************************************************************/

void Preferences::removeLanguage() {
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

void Preferences::selectedLanguageChanged(int index) {
	if (index != -1) {
		QFileInfo info("dict:" + m_languages->itemData(index).toString() + ".dic");
		m_remove_language_button->setEnabled(info.canonicalFilePath().startsWith(Dictionary::path()));
	}
}

/*****************************************************************************/

void Preferences::addWord() {
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

void Preferences::removeWord() {
	delete m_personal_dictionary->selectedItems().first();
	m_personal_dictionary->clearSelection();
}

/*****************************************************************************/

void Preferences::selectedWordChanged() {
	m_remove_word_button->setDisabled(m_personal_dictionary->selectedItems().isEmpty());
}

/*****************************************************************************/

void Preferences::wordEdited() {
	QString word = m_word->text();
	m_add_word_button->setEnabled(!word.isEmpty() && m_personal_dictionary->findItems(word, Qt::MatchExactly).isEmpty());
}

/*****************************************************************************/

QWidget* Preferences::initGeneralTab() {
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

	// Create statistics options
	QGroupBox* stats_group = new QGroupBox(tr("Statistics"), tab);

	m_show_characters = new QCheckBox(tr("Show character count"), stats_group);
	m_show_pages = new QCheckBox(tr("Show page count"), stats_group);
	m_show_paragraphs = new QCheckBox(tr("Show paragraph count"), stats_group);
	m_show_words = new QCheckBox(tr("Show word count"), stats_group);

	QVBoxLayout* stats_layout = new QVBoxLayout(stats_group);
	stats_layout->addWidget(m_show_words);
	stats_layout->addWidget(m_show_pages);
	stats_layout->addWidget(m_show_paragraphs);
	stats_layout->addWidget(m_show_characters);

	// Create edit options
	QGroupBox* edit_group = new QGroupBox(tr("Editing"), tab);

	m_always_center = new QCheckBox(tr("Always center"), edit_group);

	QFormLayout* edit_layout = new QFormLayout(edit_group);
	edit_layout->addRow(m_always_center);

	// Create save options
	QGroupBox* save_group = new QGroupBox(tr("Saving"), tab);

	m_location = new QPushButton(save_group);
	m_location->setAutoDefault(false);
	connect(m_location, SIGNAL(clicked()), this, SLOT(changeSaveLocation()));

	m_auto_save = new QCheckBox(tr("Automatically save changes"), save_group);
	m_auto_append = new QCheckBox(tr("Append filename extension"), save_group);

	QFormLayout* save_layout = new QFormLayout(save_group);
	save_layout->addRow(tr("Location:"), m_location);
	save_layout->addRow(m_auto_save);
	save_layout->addRow(m_auto_append);

	// Lay out general options
	QVBoxLayout* layout = new QVBoxLayout(tab);
	layout->addWidget(goals_group);
	layout->addWidget(stats_group);
	layout->addWidget(edit_group);
	layout->addWidget(save_group);
	layout->addStretch();

	return tab;
}

/*****************************************************************************/

QWidget* Preferences::initSpellingTab() {
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

	QProcess unzip;
	unzip.start("unzip");
	unzip.waitForFinished(-1);
	m_add_language_button->setEnabled(unzip.error() != QProcess::FailedToStart);

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
