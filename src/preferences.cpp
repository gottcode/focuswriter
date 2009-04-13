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

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

/*****************************************************************************/

Preferences::Preferences(QWidget* parent)
: QDialog(parent) {
	setWindowTitle(tr("Preferences"));

	// Create goal options
	QGroupBox* goals_group = new QGroupBox(tr("Daily Goal"), this);

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
	QGroupBox* edit_group = new QGroupBox(tr("Editing"), this);

	m_always_center = new QCheckBox(tr("Always center"), edit_group);

	QFormLayout* edit_layout = new QFormLayout(edit_group);
	edit_layout->addRow(m_always_center);

	// Create save options
	QGroupBox* save_group = new QGroupBox(tr("Saving"), this);

	m_location = new QPushButton(save_group);
	m_location->setAutoDefault(false);
	connect(m_location, SIGNAL(clicked()), this, SLOT(changeSaveLocation()));

	m_auto_save = new QCheckBox(tr("Automatically save changes"), save_group);
	m_auto_append = new QCheckBox(tr("Append filename extension"), save_group);

	QFormLayout* save_layout = new QFormLayout(save_group);
	save_layout->addRow(tr("Location:"), m_location);
	save_layout->addRow(m_auto_save);
	save_layout->addRow(m_auto_append);

	// Lay out dialog
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(goals_group);
	layout->addWidget(edit_group);
	layout->addWidget(save_group);
	layout->addWidget(buttons);

	// Load settings
	QSettings settings;
	switch (settings.value("Goal/Type").toInt()) {
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
	m_time->setValue(settings.value("Goal/Minutes", 15).toInt());
	m_wordcount->setValue(settings.value("Goal/Words", 2000).toInt());
	m_always_center->setCheckState(settings.value("Edit/AlwaysCenter", false).toBool() ? Qt::Checked : Qt::Unchecked);
	m_location->setText(settings.value("Save/Location", QDir::currentPath()).toString());
	m_auto_save->setCheckState(settings.value("Save/Auto", true).toBool() ? Qt::Checked : Qt::Unchecked);
	m_auto_append->setCheckState(settings.value("Save/Append", true).toBool() ? Qt::Checked : Qt::Unchecked);
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

bool Preferences::alwaysCenter() const {
	return m_always_center->checkState() == Qt::Checked;
}

/*****************************************************************************/

QString Preferences::saveLocation() const {
	return m_location->text();
}

/*****************************************************************************/

bool Preferences::autoSave() const {
	return m_auto_save->checkState() == Qt::Checked;
}

/*****************************************************************************/

bool Preferences::autoAppend() const {
	return m_auto_append->checkState() == Qt::Checked;
}

/*****************************************************************************/

void Preferences::accept() {
	QSettings settings;
	settings.setValue("Goal/Type", goalType());
	settings.setValue("Goal/Minutes", goalMinutes());
	settings.setValue("Goal/Words", goalWords());
	settings.setValue("Edit/AlwaysCenter", alwaysCenter());
	settings.setValue("Save/Auto", autoSave());
	settings.setValue("Save/Append", autoAppend());
	settings.setValue("Save/Location", m_location->text());
	QDir::setCurrent(m_location->text());
	QDialog::accept();
}

/*****************************************************************************/

void Preferences::changeSaveLocation() {
	QString location = QFileDialog::getExistingDirectory(this, tr("Find Directory"), m_location->text());
	if (!location.isEmpty()) {
		m_location->setText(location);
	}
}

/*****************************************************************************/
