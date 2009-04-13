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
#include <QSettings>
#include <QVBoxLayout>

/*****************************************************************************/

Preferences::Preferences(QWidget* parent)
: QDialog(parent) {
	setWindowTitle(tr("Preferences"));

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
	layout->addWidget(edit_group);
	layout->addWidget(save_group);
	layout->addWidget(buttons);

	// Load settings
	QSettings settings;
	m_always_center->setCheckState(settings.value("Edit/AlwaysCenter", false).toBool() ? Qt::Checked : Qt::Unchecked);
	m_location->setText(settings.value("Save/Location", QDir::currentPath()).toString());
	m_auto_save->setCheckState(settings.value("Save/Auto", true).toBool() ? Qt::Checked : Qt::Unchecked);
	m_auto_append->setCheckState(settings.value("Save/Append", true).toBool() ? Qt::Checked : Qt::Unchecked);
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
