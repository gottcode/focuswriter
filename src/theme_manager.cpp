/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
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

#include "theme_manager.h"

#include "theme.h"
#include "theme_dialog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

/*****************************************************************************/

ThemeManager::ThemeManager(QWidget* parent)
: QDialog(parent) {
	setWindowTitle(tr("Themes"));

	// Add themes list
	m_themes = new QListWidget(this);
	m_themes->setSortingEnabled(true);
	m_themes->setSpacing(12);
	m_themes->setViewMode(QListView::IconMode);
	m_themes->setIconSize(QSize(218, 168));
	m_themes->setMovement(QListView::Static);
	m_themes->setResizeMode(QListView::Adjust);
	m_themes->setSelectionMode(QAbstractItemView::SingleSelection);
	m_themes->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_themes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_themes->setMinimumSize(250, 250);
	m_themes->setWordWrap(true);
	QStringList themes = QDir(Theme::path(), "*.theme").entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
	foreach (const QString& theme, themes) {
		addItem(QFileInfo(theme).baseName());
	}
	QList<QListWidgetItem*> items = m_themes->findItems(QSettings().value("ThemeManager/Theme").toString(), Qt::MatchExactly);
	if (!items.isEmpty()) {
		m_themes->setCurrentItem(items.first());
	}
	connect(m_themes, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(currentThemeChanged(QListWidgetItem*)));
	connect(m_themes, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(modifyTheme()));

	// Add control buttons
	QPushButton* add_button = new QPushButton(tr("Add"), this);
	add_button->setAutoDefault(false);
	connect(add_button, SIGNAL(clicked()), this, SLOT(addTheme()));

	QPushButton* edit_button = new QPushButton(tr("Modify"), this);
	edit_button->setAutoDefault(false);
	connect(edit_button, SIGNAL(clicked()), this, SLOT(modifyTheme()));

	QPushButton* remove_button = new QPushButton(tr("Remove"), this);
	remove_button->setAutoDefault(false);
	connect(remove_button, SIGNAL(clicked()), this, SLOT(removeTheme()));

	QPushButton* close_button = new QPushButton(tr("Close"), this);
	close_button->setAutoDefault(false);
	connect(close_button, SIGNAL(clicked()), this, SLOT(accept()));

	// Lay out dialog
	QVBoxLayout* buttons_layout = new QVBoxLayout;
	buttons_layout->setMargin(0);
	buttons_layout->addWidget(add_button);
	buttons_layout->addWidget(edit_button);
	buttons_layout->addWidget(remove_button);
	buttons_layout->addStretch();
	buttons_layout->addWidget(close_button);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(m_themes);
	layout->addLayout(buttons_layout);

	// Restore size
	resize(QSettings().value("ThemeManager/Size", QSize(630, 450)).toSize());
}

/*****************************************************************************/

void ThemeManager::accept() {
	QList<QListWidgetItem*> items = m_themes->selectedItems();
	if (!items.isEmpty()) {
		QSettings().setValue("ThemeManager/Theme", items.first()->text());
	}
	QDialog::accept();
}

/*****************************************************************************/

void ThemeManager::hideEvent(QHideEvent* event) {
	QSettings().setValue("ThemeManager/Size", size());
	QDialog::hideEvent(event);
}

/*****************************************************************************/

void ThemeManager::addTheme() {
	QString name;
	{
		Theme theme;
		ThemeDialog dialog(theme, this);
		if (dialog.exec() == QDialog::Rejected) {
			return;
		}
		name = theme.name();
	}
	addItem(name);
}

/*****************************************************************************/

void ThemeManager::modifyTheme() {
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	QString name;
	{
		Theme theme(item->text());
		ThemeDialog dialog(theme, this);
		if (dialog.exec() == QDialog::Rejected) {
			return;
		}
		name = theme.name();
	}
	if (name == item->text()) {
		item->setIcon(QIcon(Theme::iconPath(name)));
		emit themeSelected(Theme(name));
	} else {
		delete item;
		item = 0;
		addItem(name);
	}
}

/*****************************************************************************/

void ThemeManager::removeTheme() {
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Question"), tr("Remove selected theme?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
		QFile::remove(Theme::filePath(item->text()));
		QFile::remove(Theme::iconPath(item->text()));
		delete item;
		item = 0;
	}
}

/*****************************************************************************/

void ThemeManager::currentThemeChanged(QListWidgetItem* current) {
	if (current) {
		Theme theme(current->text());
		emit themeSelected(theme);
	}
}

/*****************************************************************************/

void ThemeManager::addItem(const QString& name) {
	QString icon = Theme::iconPath(name);
	if (!QFile::exists(icon)) {
		ThemeDialog::createPreview(name);
	}
	QListWidgetItem* item = new QListWidgetItem(QIcon(icon), name, m_themes);
	m_themes->setCurrentItem(item);
}

/*****************************************************************************/
