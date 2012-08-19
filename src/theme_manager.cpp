/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "gzip.h"
#include "theme.h"
#include "theme_dialog.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryFile>
#include <QUrl>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

ThemeManager::ThemeManager(QSettings& settings, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_settings(settings)
{
	setWindowTitle(tr("Themes"));

	// Add themes list
	m_themes = new QListWidget(this);
	m_themes->setSortingEnabled(true);
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
		addItem(QUrl::fromPercentEncoding(QFileInfo(theme).baseName().toUtf8()));
	}
	QList<QListWidgetItem*> items = m_themes->findItems(m_settings.value("ThemeManager/Theme").toString(), Qt::MatchExactly);
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

	QPushButton* import_button = new QPushButton(tr("Import"), this);
	import_button->setAutoDefault(false);
	connect(import_button, SIGNAL(clicked()), this, SLOT(importTheme()));

	QPushButton* export_button = new QPushButton(tr("Export"), this);
	export_button->setAutoDefault(false);
	connect(export_button, SIGNAL(clicked()), this, SLOT(exportTheme()));

	QPushButton* close_button = new QPushButton(tr("Close"), this);
	close_button->setAutoDefault(false);
	connect(close_button, SIGNAL(clicked()), this, SLOT(accept()));

	// Lay out dialog
	QVBoxLayout* buttons_layout = new QVBoxLayout;
	buttons_layout->setMargin(0);
	buttons_layout->addWidget(add_button);
	buttons_layout->addWidget(edit_button);
	buttons_layout->addWidget(remove_button);
	buttons_layout->addSpacing(import_button->sizeHint().height());
	buttons_layout->addWidget(import_button);
	buttons_layout->addWidget(export_button);
	buttons_layout->addStretch();
	buttons_layout->addWidget(close_button);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(m_themes);
	layout->addLayout(buttons_layout);

	// Restore size
	resize(m_settings.value("ThemeManager/Size", QSize(630, 450)).toSize());
}

//-----------------------------------------------------------------------------

void ThemeManager::hideEvent(QHideEvent* event)
{
	QList<QListWidgetItem*> items = m_themes->selectedItems();
	QString selected = !items.isEmpty() ? items.first()->text() : QString();
	if (!selected.isEmpty()) {
		m_settings.setValue("ThemeManager/Theme", selected);
	}
	m_settings.setValue("ThemeManager/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void ThemeManager::addTheme()
{
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

//-----------------------------------------------------------------------------

void ThemeManager::modifyTheme()
{
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
		emit themeSelected(name);
	} else {
		delete item;
		item = 0;
		addItem(name);
	}
}

//-----------------------------------------------------------------------------

void ThemeManager::removeTheme()
{
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

//-----------------------------------------------------------------------------

void ThemeManager::importTheme()
{
	// Find file to import
	QString filename = QFileDialog::getOpenFileName(this, tr("Import Theme"), QDir::homePath(), tr("Themes (*.fwtz *.theme)"));
	if (filename.isEmpty()) {
		return;
	}

	// Find theme name
	QString name = QUrl::fromPercentEncoding(QFileInfo(filename).baseName().toUtf8());
	while (QFile::exists(Theme::filePath(name))) {
		bool ok;
		name = QInputDialog::getText(this, tr("Sorry"), tr("A theme already exists with that name. Please enter a new name:"), QLineEdit::Normal, name, &ok, 0, Qt::ImhNone);
		if (!ok) {
			return;
		}
	}

	// Uncompress theme
	QString theme_filename = Theme::filePath(name);
	QByteArray theme = gunzip(filename);
	{
		QFile file(theme_filename);
		if (file.open(QFile::WriteOnly)) {
			file.write(theme);
			file.close();
		}
	}

	// Extract and use background image
	QSettings settings(theme_filename, QSettings::IniFormat);
	QByteArray data = QByteArray::fromBase64(settings.value("Data/Image").toByteArray());
	QString image_file = settings.value("Background/ImageFile").toString();
	settings.remove("Background/ImageFile");
	settings.remove("Data/Image");
	settings.sync();

	if (!data.isEmpty()) {
		QTemporaryFile file(QDir::tempPath() + "/XXXXXX-" + image_file);
		if (file.open()) {
			file.write(data);
			file.close();
		}

		Theme theme(name);
		theme.setBackgroundImage(file.fileName());
	}

	settings.sync();
	settings.remove("Background/Image");

	addItem(name);
}

//-----------------------------------------------------------------------------

void ThemeManager::exportTheme()
{
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	// Find export file name
	QString filename = QFileDialog::getSaveFileName(this, tr("Export Theme"), QDir::homePath() + "/" + item->text() + ".fwtz", tr("Themes (*.fwtz)"));
	if (filename.isEmpty()) {
		return;
	}
	if (!filename.endsWith(".fwtz")) {
		filename += ".fwtz";
	}

	// Copy theme
	QFile::remove(filename);
	QFile::copy(Theme::filePath(item->text()), filename);

	// Store image in export file
	{
		QSettings settings(filename, QSettings::IniFormat);
		settings.remove("Background/Image");

		QString image = settings.value("Background/ImageFile").toString();
		if (!image.isEmpty()) {
			QFile file(Theme::path() + "/Images/" + image);
			if (file.open(QFile::ReadOnly)) {
				settings.setValue("Data/Image", file.readAll().toBase64());
				file.close();
			}
		}
	}

	// Compress theme
	gzip(filename);
}

//-----------------------------------------------------------------------------

void ThemeManager::currentThemeChanged(QListWidgetItem* current)
{
	if (current) {
		emit themeSelected(current->text());
	}
}

//-----------------------------------------------------------------------------

void ThemeManager::addItem(const QString& name)
{
	QString icon = Theme::iconPath(name);
	if (!QFile::exists(icon)) {
		ThemeDialog::createPreview(name);
	}
	QListWidgetItem* item = new QListWidgetItem(QIcon(icon), name, m_themes);
	m_themes->setCurrentItem(item);
}

//-----------------------------------------------------------------------------
