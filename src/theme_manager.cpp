/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012, 2014 Graeme Gott <graeme@gottcode.org>
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
#include "session.h"
#include "theme.h"
#include "theme_dialog.h"
#include "utils.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif
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
	m_themes->setIconSize(QSize(258, 153));
	m_themes->setMovement(QListView::Static);
	m_themes->setResizeMode(QListView::Adjust);
	m_themes->setSelectionMode(QAbstractItemView::SingleSelection);
	m_themes->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_themes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_themes->setMinimumSize(250, 250);
	m_themes->setWordWrap(true);
	QStringList themes = QDir(Theme::path(), "*.theme").entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
	foreach (const QString& theme, themes) {
		addItem(QUrl::fromPercentEncoding(QFileInfo(theme).completeBaseName().toUtf8()));
	}
	QList<QListWidgetItem*> items = m_themes->findItems(m_settings.value("ThemeManager/Theme").toString(), Qt::MatchExactly);
	if (!items.isEmpty()) {
		m_themes->setCurrentItem(items.first());
	}
	connect(m_themes, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(currentThemeChanged(QListWidgetItem*)));
	connect(m_themes, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(editTheme()));

	// Add control buttons
	QPushButton* new_button = new QPushButton(tr("New"), this);
	new_button->setAutoDefault(false);
	connect(new_button, SIGNAL(clicked()), this, SLOT(newTheme()));

	QPushButton* clone_button = new QPushButton(tr("Duplicate"), this);
	clone_button->setAutoDefault(false);
	connect(clone_button, SIGNAL(clicked()), this, SLOT(cloneTheme()));

	QPushButton* edit_button = new QPushButton(tr("Edit"), this);
	edit_button->setAutoDefault(false);
	connect(edit_button, SIGNAL(clicked()), this, SLOT(editTheme()));

	m_remove_button = new QPushButton(tr("Delete"), this);
	m_remove_button->setAutoDefault(false);
	connect(m_remove_button, SIGNAL(clicked()), this, SLOT(deleteTheme()));

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
	buttons_layout->addWidget(new_button);
	buttons_layout->addWidget(clone_button);
	buttons_layout->addWidget(edit_button);
	buttons_layout->addWidget(m_remove_button);
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

void ThemeManager::newTheme()
{
	QString name;
	{
		Theme theme(QString(), false);
		ThemeDialog dialog(theme, this);
		dialog.setWindowTitle(ThemeDialog::tr("New Theme"));
		if (dialog.exec() == QDialog::Rejected) {
			return;
		}
		name = theme.name();
	}
	addItem(name);
	m_remove_button->setEnabled(true);
}

//-----------------------------------------------------------------------------

void ThemeManager::editTheme()
{
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	QString name;
	{
		Theme theme(item->text(), false);
		ThemeDialog dialog(theme, this);
		if (dialog.exec() == QDialog::Rejected) {
			return;
		}
		name = theme.name();
	}
	if (name == item->text()) {
		item->setIcon(QIcon(Theme::iconPath(name)));
		emit themeSelected(Theme(name, false));
	} else {
		delete item;
		item = 0;
		addItem(name);
	}
}

//-----------------------------------------------------------------------------

void ThemeManager::cloneTheme()
{
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	QString name = Theme::clone(item->text(), false);
	{
		Theme theme(name, false);
		ThemeDialog dialog(theme, this);
		dialog.setWindowTitle(ThemeDialog::tr("New Theme"));
		if (dialog.exec() == QDialog::Rejected) {
			QFile::remove(Theme::filePath(name));
			QFile::remove(Theme::iconPath(name));
			return;
		}
		name = theme.name();
	}
	addItem(name);
}

//-----------------------------------------------------------------------------

void ThemeManager::deleteTheme()
{
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Question"), tr("Delete theme '%1'?").arg(item->text()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
		QFile::remove(Theme::filePath(item->text()));
		QFile::remove(Theme::iconPath(item->text()));
		delete item;
		item = 0;

		// Create default theme if all themes are removed
		if (m_themes->count() == 0) {
			Theme theme;
			theme.setName(Session::tr("Default"));
			addItem(theme.name());
			m_remove_button->setDisabled(true);
		}
	}
}

//-----------------------------------------------------------------------------

void ThemeManager::importTheme()
{
	// Find file to import
	QSettings settings;
	QString path = settings.value("ThemeManager/Location").toString();
	if (path.isEmpty() || !QFile::exists(path)) {
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
		path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
	}
	QString filename = QFileDialog::getOpenFileName(this, tr("Import Theme"), path, tr("Themes (%1)").arg("*.fwtz *.theme"), 0, QFileDialog::DontResolveSymlinks);
	if (filename.isEmpty()) {
		return;
	}
	settings.setValue("ThemeManager/Location", QFileInfo(filename).absolutePath());

	// Find theme name
	QString name = QUrl::fromPercentEncoding(QFileInfo(filename).completeBaseName().toUtf8());
	{
		QStringList values = splitStringAtLastNumber(name);
		int count = values.at(1).toInt();
		while (QFile::exists(Theme::filePath(name))) {
			++count;
			name = values.at(0) + QString::number(count);
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
	QSettings theme_ini(theme_filename, QSettings::IniFormat);
	QByteArray data = QByteArray::fromBase64(theme_ini.value("Data/Image").toByteArray());
	QString image_file = theme_ini.value("Background/ImageFile").toString();
	theme_ini.remove("Background/ImageFile");
	theme_ini.remove("Data/Image");
	theme_ini.sync();

	if (!data.isEmpty()) {
		QTemporaryFile file(QDir::tempPath() + "/XXXXXX-" + image_file);
		if (file.open()) {
			file.write(data);
			file.close();
		}

		Theme theme(name, false);
		theme.setBackgroundImage(file.fileName());
	}

	theme_ini.sync();
	theme_ini.remove("Background/Image");

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
	QSettings settings;
	QString path = settings.value("ThemeManager/Location").toString();
	if (path.isEmpty() || !QFile::exists(path)) {
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#else
		path = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#endif
	}
	path = path + "/" + item->text() + ".fwtz";
	QString filename = QFileDialog::getSaveFileName(this, tr("Export Theme"), path, tr("Themes (%1)").arg("*.fwtz"), 0, QFileDialog::DontResolveSymlinks);
	if (filename.isEmpty()) {
		return;
	}
	if (!filename.endsWith(".fwtz")) {
		filename += ".fwtz";
	}
	settings.setValue("ThemeManager/Location", QFileInfo(filename).absolutePath());

	// Copy theme
	QFile::remove(filename);
	QFile::copy(Theme::filePath(item->text()), filename);

	// Store image in export file
	{
		QSettings theme_ini(filename, QSettings::IniFormat);
		theme_ini.remove("Background/Image");

		QString image = theme_ini.value("Background/ImageFile").toString();
		if (!image.isEmpty()) {
			QFile file(Theme::path() + "/Images/" + image);
			if (file.open(QFile::ReadOnly)) {
				theme_ini.setValue("Data/Image", file.readAll().toBase64());
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
		emit themeSelected(Theme(current->text(), false));
	}
}

//-----------------------------------------------------------------------------

void ThemeManager::addItem(const QString& name)
{
	QString icon = Theme::iconPath(name);
	if (!QFile::exists(icon) || QImageReader(icon).size() != QSize(258, 153)) {
		ThemeDialog::createPreview(name, false);
	}
	QListWidgetItem* item = new QListWidgetItem(QIcon(icon), name, m_themes);
	m_themes->setCurrentItem(item);
}

//-----------------------------------------------------------------------------
