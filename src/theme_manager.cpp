/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012, 2014, 2016, 2018, 2019 Graeme Gott <graeme@gottcode.org>
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

#include <QApplication>
#include <QDialogButtonBox>
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
#include <QStandardPaths>
#include <QStyle>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QUrl>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

namespace
{

class ThemeItem : public QListWidgetItem
{
public:
	ThemeItem(const QIcon& icon, const QString& text, QListWidget* view) :
		QListWidgetItem(icon, text, view)
	{
	}

	bool operator<(const QListWidgetItem& other) const
	{
		return localeAwareSort(text(), other.text());
	}
};

}

//-----------------------------------------------------------------------------

ThemeManager::ThemeManager(QSettings& settings, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_settings(settings)
{
	setWindowTitle(tr("Themes"));

	m_tabs = new QTabWidget(this);

	// Find view sizes
	int focush = style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
	int focusv = style()->pixelMetric(QStyle::PM_FocusFrameVMargin);
	int frame = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	int scrollbar = style()->pixelMetric(QStyle::PM_SliderThickness);
	QSize grid_size(259 + focush, 154 + focusv + (fontMetrics().height() * 2));
	QSize view_size((grid_size.width() + frame + focush) * 2 + scrollbar, (grid_size.height() + frame + focusv) * 2);

	// Add default themes tab
	QWidget* tab = new QWidget(this);
	m_tabs->addTab(tab, tr("Default"));

	// Add default themes list
	m_default_themes = new QListWidget(tab);
	m_default_themes->setSortingEnabled(true);
	m_default_themes->setViewMode(QListView::IconMode);
	m_default_themes->setIconSize(QSize(258, 153));
	m_default_themes->setGridSize(grid_size);
	m_default_themes->setMovement(QListView::Static);
	m_default_themes->setResizeMode(QListView::Adjust);
	m_default_themes->setSelectionMode(QAbstractItemView::SingleSelection);
	m_default_themes->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_default_themes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_default_themes->setMinimumSize(view_size);
	m_default_themes->setWordWrap(true);
	addItem("bitterskies", true, tr("Bitter Skies"));
	addItem("enchantment", true, tr("Enchantment"));
	addItem("gentleblues", true, tr("Gentle Blues"));
	addItem("oldschool", true, tr("Old School"));
	addItem("spacedreams", true, tr("Space Dreams"));
	addItem("spygames", true, tr("Spy Games"));
	addItem("tranquility", true, tr("Tranquility"));
	addItem("writingdesk", true, tr("Writing Desk"));

	// Add default control buttons
	QPushButton* new_default_button = new QPushButton(tr("New"), tab);
	new_default_button->setAutoDefault(false);
	connect(new_default_button, &QPushButton::clicked, this, &ThemeManager::newTheme);

	m_clone_default_button = new QPushButton(tr("Duplicate"), tab);
	m_clone_default_button->setAutoDefault(false);
	connect(m_clone_default_button, &QPushButton::clicked, this, &ThemeManager::cloneTheme);

	// Lay out default themes tab
	QGridLayout* default_layout = new QGridLayout(tab);
	default_layout->setColumnStretch(0, 1);
	default_layout->setRowStretch(2, 1);
	default_layout->addWidget(m_default_themes, 0, 0, 3, 1);
	default_layout->addWidget(new_default_button, 0, 1);
	default_layout->addWidget(m_clone_default_button, 1, 1);

	// Add themes tab
	tab = new QWidget(this);
	m_tabs->addTab(tab, tr("Custom"));

	// Add themes list
	m_themes = new QListWidget(tab);
	m_themes->setSortingEnabled(true);
	m_themes->setViewMode(QListView::IconMode);
	m_themes->setIconSize(QSize(258, 153));
	m_themes->setGridSize(grid_size);
	m_themes->setMovement(QListView::Static);
	m_themes->setResizeMode(QListView::Adjust);
	m_themes->setSelectionMode(QAbstractItemView::SingleSelection);
	m_themes->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_themes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_themes->setMinimumSize(view_size);
	m_themes->setWordWrap(true);
	QDir dir(Theme::path(), "*.theme");
	QStringList themes = dir.entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
	for (const QString& theme : themes) {
		QString name = QSettings(dir.filePath(theme), QSettings::IniFormat).value("Name").toString();
		if (!name.isEmpty()) {
			addItem(QFileInfo(theme).completeBaseName(), false, name);
		} else {
			name = QUrl::fromPercentEncoding(QFileInfo(theme).completeBaseName().toUtf8());
			QSettings(dir.filePath(theme), QSettings::IniFormat).setValue("Name", name);

			QString id = Theme::createId();
			dir.rename(theme, id + ".theme");
			dir.remove(QFileInfo(theme).completeBaseName() + ".png");

			QStringList sessions = QDir(Session::path(), "*.session").entryList(QDir::Files);
			sessions.prepend("");
			for (const QString& file : sessions) {
				Session session(file);
				if ((session.theme() == name) && (session.themeDefault() == false)) {
					session.setTheme(id, false);
				}
			}

			addItem(id, false, name);
		}
	}

	// Add control buttons
	QPushButton* new_button = new QPushButton(tr("New"), tab);
	new_button->setAutoDefault(false);
	connect(new_button, &QPushButton::clicked, this, &ThemeManager::newTheme);

	m_clone_button = new QPushButton(tr("Duplicate"), tab);
	m_clone_button->setAutoDefault(false);
	m_clone_button->setEnabled(false);
	connect(m_clone_button, &QPushButton::clicked, this, &ThemeManager::cloneTheme);

	m_edit_button = new QPushButton(tr("Edit"), tab);
	m_edit_button->setAutoDefault(false);
	m_edit_button->setEnabled(false);
	connect(m_edit_button, &QPushButton::clicked, this, &ThemeManager::editTheme);

	m_remove_button = new QPushButton(tr("Delete"), tab);
	m_remove_button->setAutoDefault(false);
	m_remove_button->setEnabled(false);
	connect(m_remove_button, &QPushButton::clicked, this, &ThemeManager::deleteTheme);

	QPushButton* import_button = new QPushButton(tr("Import"), tab);
	import_button->setAutoDefault(false);
	connect(import_button, &QPushButton::clicked, this, &ThemeManager::importTheme);

	m_export_button = new QPushButton(tr("Export"), tab);
	m_export_button->setAutoDefault(false);
	m_export_button->setEnabled(false);
	connect(m_export_button, &QPushButton::clicked, this, &ThemeManager::exportTheme);

	// Lay out custom themes tab
	QGridLayout* custom_layout = new QGridLayout(tab);
	custom_layout->setColumnStretch(0, 1);
	custom_layout->setRowMinimumHeight(4, import_button->sizeHint().height());
	custom_layout->setRowStretch(7, 1);
	custom_layout->addWidget(m_themes, 0, 0, 8, 1);
	custom_layout->addWidget(new_button, 0, 1);
	custom_layout->addWidget(m_clone_button, 1, 1);
	custom_layout->addWidget(m_edit_button, 2, 1);
	custom_layout->addWidget(m_remove_button, 3, 1);
	custom_layout->addWidget(import_button, 5, 1);
	custom_layout->addWidget(m_export_button, 6, 1);

	// Lay out dialog
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &ThemeManager::reject);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_tabs, 1);
	layout->addSpacing(layout->contentsMargins().top());
	layout->addWidget(buttons);

	// Select theme
	QString theme = m_settings.value("ThemeManager/Theme").toString();
	bool is_default = m_settings.value("ThemeManager/ThemeDefault", false).toBool();
	if (!selectItem(theme, is_default)) {
		selectItem(Theme::defaultId(), true);
	}
	selectionChanged(is_default);
	connect(m_default_themes, &QListWidget::currentItemChanged, this, &ThemeManager::currentThemeChanged);
	connect(m_themes, &QListWidget::currentItemChanged, this, &ThemeManager::currentThemeChanged);
	connect(m_themes, &QListWidget::itemActivated, this, &ThemeManager::editTheme);

	// Restore size
	resize(m_settings.value("ThemeManager/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

void ThemeManager::hideEvent(QHideEvent* event)
{
	m_settings.setValue("ThemeManager/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void ThemeManager::newTheme()
{
	Theme theme(QString(), false);
	ThemeDialog dialog(theme, this);
	dialog.setWindowTitle(ThemeDialog::tr("New Theme"));
	if (dialog.exec() == QDialog::Rejected) {
		return;
	}

	QListWidgetItem* item = addItem(theme.id(), false, theme.name());
	m_themes->setCurrentItem(item);

	m_tabs->setCurrentIndex(1);
}

//-----------------------------------------------------------------------------

void ThemeManager::editTheme()
{
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	Theme theme(item->data(Qt::UserRole).toString(), false);
	ThemeDialog dialog(theme, this);
	if (dialog.exec() == QDialog::Rejected) {
		return;
	}

	item->setText(theme.name());
	item->setIcon(QIcon(Theme::iconPath(theme.id(), false, devicePixelRatioF())));
	emit themeSelected(theme);
}

//-----------------------------------------------------------------------------

void ThemeManager::cloneTheme()
{
	bool is_default = m_tabs->currentIndex() == 0;
	QListWidgetItem* item = (is_default ? m_default_themes : m_themes)->currentItem();
	if (!item) {
		return;
	}

	QString id = Theme::clone(item->data(Qt::UserRole).toString(), is_default, item->text());
	QString name = QSettings(Theme::filePath(id, false), QSettings::IniFormat).value("Name").toString();
	item = addItem(id, false, name);
	m_themes->setCurrentItem(item);

	m_tabs->setCurrentIndex(1);
}

//-----------------------------------------------------------------------------

void ThemeManager::deleteTheme()
{
	QListWidgetItem* item = m_themes->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Question"), tr("Delete theme '%1'?").arg(item->text()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
		QString id = item->data(Qt::UserRole).toString();
		QFile::remove(Theme::filePath(id));
		Theme::removeIcon(id, false);
		delete item;
		item = 0;

		// Handle deleting last custom theme
		if (m_themes->count() == 0) {
			selectItem(Theme::defaultId(), true);
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
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	}
	QString filename = QFileDialog::getOpenFileName(this, tr("Import Theme"), path, tr("Themes (%1)").arg("*.fwtz *.theme"));
	if (filename.isEmpty()) {
		return;
	}
	settings.setValue("ThemeManager/Location", QFileInfo(filename).absolutePath());

	QString id = Theme::createId();

	// Uncompress theme
	QString theme_filename = Theme::filePath(id);
	QByteArray theme = gunzip(filename);
	{
		QFile file(theme_filename);
		if (file.open(QFile::WriteOnly)) {
			file.write(theme);
			file.close();
		}
	}

	// Find theme name
	QSettings theme_ini(theme_filename, QSettings::IniFormat);
	QString name = theme_ini.value("Name", QFileInfo(filename).completeBaseName()).toString();
	{
		QStringList values = splitStringAtLastNumber(name);
		int count = values.at(1).toInt();
		while (Theme::exists(name)) {
			++count;
			name = values.at(0) + QString::number(count);
		}
		theme_ini.setValue("Name", name);
	}

	// Extract and use background image
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

		Theme theme(id, false);
		theme.setBackgroundImage(file.fileName());
		theme.saveChanges();
	}

	theme_ini.sync();
	theme_ini.remove("Background/Image");

	QListWidgetItem* item = addItem(id, false, name);
	m_themes->setCurrentItem(item);
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
		path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
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
	QFile::copy(Theme::filePath(item->data(Qt::UserRole).toString()), filename);

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
		bool is_default = current->listWidget() == m_default_themes;
		if (is_default) {
			m_themes->setCurrentIndex(m_themes->rootIndex());
		} else {
			m_default_themes->setCurrentIndex(m_default_themes->rootIndex());
		}

		selectionChanged(is_default);

		QString id = current->data(Qt::UserRole).toString();
		m_settings.setValue("ThemeManager/Theme", id);
		m_settings.setValue("ThemeManager/ThemeDefault", is_default);
		emit themeSelected(Theme(id, is_default));
	}
}

//-----------------------------------------------------------------------------

QListWidgetItem* ThemeManager::addItem(const QString& id, bool is_default, const QString& name)
{
	const qreal pixelratio = devicePixelRatioF();
	QString icon = Theme::iconPath(id, is_default, pixelratio);
	if (!QFile::exists(icon) || QImageReader(icon).size() != (QSize(258, 153) * pixelratio)) {
		Theme theme(id, is_default);

		// Find load color in separate thread
		QFuture<QColor> load_color;
		if (!theme.isDefault() && (theme.loadColor() == theme.backgroundColor())) {
			load_color = theme.calculateLoadColor();
		}

		// Generate preview
		QRect foreground;
		QImage background = theme.render(QSize(1920, 1080), foreground, 0, pixelratio);
		QImage icon;
		theme.renderText(background, foreground, pixelratio, nullptr, &icon);
		icon.save(Theme::iconPath(theme.id(), theme.isDefault(), pixelratio));

		// Save load color
		load_color.waitForFinished();
		if (load_color.resultCount()) {
			theme.setLoadColor(load_color);
			theme.saveChanges();
		}

		QApplication::processEvents();
	}

	QListWidgetItem* item = new ThemeItem(QIcon(icon), name, is_default ? m_default_themes : m_themes);
	item->setToolTip(name);
	item->setData(Qt::UserRole, id);
	return item;
}

//-----------------------------------------------------------------------------

bool ThemeManager::selectItem(const QString& id, bool is_default)
{
	QListWidget* view = m_themes;
	QListWidget* other_view = m_default_themes;
	if (is_default) {
		std::swap(view, other_view);
	}
	QAbstractItemModel* model = view->model();
	QModelIndexList items = model->match(model->index(0, 0, QModelIndex()),
			Qt::UserRole, id, 1, Qt::MatchFixedString | Qt::MatchCaseSensitive);
	if (!items.isEmpty()) {
		view->setCurrentRow(items.first().row());
		other_view->setCurrentIndex(other_view->rootIndex());
		m_tabs->setCurrentIndex(!is_default);
		return true;
	} else {
		return false;
	}
}

//-----------------------------------------------------------------------------

void ThemeManager::selectionChanged(bool is_default)
{
	m_clone_default_button->setEnabled(is_default);
	m_clone_button->setDisabled(is_default);
	m_edit_button->setDisabled(is_default);
	m_remove_button->setDisabled(is_default);
	m_export_button->setDisabled(is_default);
}

//-----------------------------------------------------------------------------
