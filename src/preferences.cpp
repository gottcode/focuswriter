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

#include "color_button.h"
#include "image_button.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

/*****************************************************************************/

Preferences::Preferences(QWidget* parent)
: QDialog(parent) {
	setWindowTitle(tr("Preferences"));

	QSettings settings;

	QTabWidget* tabs = new QTabWidget(this);
	connect(this, SIGNAL(finished(int)), tabs, SLOT(setCurrentIndex(int)));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(12);
	layout->setSpacing(24);
	layout->addWidget(tabs);
	layout->addWidget(buttons);


	// Create page tab
	QWidget* tab = new QWidget(this);
	tabs->addTab(tab, tr("Page"));

	// Set up page colors
	m_text_color = new ColorButton(settings.value("Text/Color", "#000000").toString(), tab);
	connect(m_text_color, SIGNAL(changed(const QColor&)), this, SLOT(updateColors()));

	m_page_color = new ColorButton(settings.value("Page/Color", "#cccccc").toString(), tab);
	connect(m_page_color, SIGNAL(changed(const QColor&)), this, SLOT(updateColors()));

	// Set up page opacity
	m_page_opacity_slider = new QSlider(Qt::Horizontal, tab);
	m_page_opacity_slider->setRange(0, 100);
	m_page_opacity_slider->setValue(settings.value("Page/Opacity", 100).toInt());
	connect(m_page_opacity_slider, SIGNAL(valueChanged(int)), this, SLOT(updateColors()));

	m_page_opacity_spinbox = new QSpinBox(tab);
	m_page_opacity_spinbox->setSuffix("%");
	m_page_opacity_spinbox->setRange(0, 100);
	m_page_opacity_spinbox->setValue(m_page_opacity_slider->value());
	connect(m_page_opacity_spinbox, SIGNAL(valueChanged(int)), this, SLOT(updateColors()));

	connect(m_page_opacity_slider, SIGNAL(valueChanged(int)), m_page_opacity_spinbox, SLOT(setValue(int)));
	connect(m_page_opacity_spinbox, SIGNAL(valueChanged(int)), m_page_opacity_slider, SLOT(setValue(int)));

	// Set up font
	QFont f;
	f.fromString(settings.value("Text/Font", font().toString()).toString());
	m_font_names = new QFontComboBox(tab);
	m_font_names->setEditable(false);
	m_font_names->setCurrentFont(f);
	connect(m_font_names, SIGNAL(currentFontChanged(const QFont&)), this, SLOT(updateFont()));

	m_font_sizes = new QComboBox(tab);
	foreach (int size, QFontDatabase::standardSizes()) {
		m_font_sizes->addItem(QString::number(size), size);
	}
	m_font_sizes->setCurrentIndex(m_font_sizes->findData(f.pointSize()));
	connect(m_font_sizes, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFont()));

	// Set up page width
	m_page_width = new QSpinBox(tab);
	m_page_width->setSuffix(tr(" pixels"));
	m_page_width->setRange(500, 2000);
	m_page_width->setValue(settings.value("Page/Width", 700).toInt());
	connect(m_page_width, SIGNAL(valueChanged(int)), this, SLOT(updateWidth()));

	// Lay out tab
	QGridLayout* tab_layout = new QGridLayout(tab);
	tab_layout->setMargin(12);
	tab_layout->setSpacing(6);
	tab_layout->setColumnStretch(3, 1);
	tab_layout->setRowStretch(0, 1);
	tab_layout->setRowStretch(6, 1);
	tab_layout->addWidget(new QLabel(tr("Text color:"), this), 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_text_color, 1, 2);
	tab_layout->addWidget(new QLabel(tr("Page color:"), this), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_page_color, 2, 2);
	tab_layout->addWidget(new QLabel(tr("Opacity:"), this), 3, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_page_opacity_slider, 3, 2, 1, 2);
	tab_layout->addWidget(m_page_opacity_spinbox, 3, 4);
	tab_layout->addWidget(new QLabel(tr("Font:"), this), 4, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_font_names, 4, 2, 1, 2);
	tab_layout->addWidget(m_font_sizes, 4, 4);
	tab_layout->addWidget(new QLabel(tr("Width:"), this), 5, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_page_width, 5, 2);


	// Create background tab
	tab = new QWidget(this);
	tabs->addTab(tab, tr("Background"));

	m_background_color = new ColorButton(settings.value("Background/Color", "#cccccc").toString(), tab);
	connect(m_background_color, SIGNAL(changed(const QColor&)), this, SLOT(updateBackground()));

	m_background_image = new ImageButton(settings.value("Background/Image").toString(), tab);
	connect(m_background_image, SIGNAL(changed(const QImage&)), this, SLOT(updateBackground()));

	m_background_position = new QComboBox(tab);
	m_background_position->addItems(QStringList() << tr("Solid Color") << tr("Tiled") << tr("Centered") << tr("Stretched") << tr("Scaled") << tr("Zoomed"));
	m_background_position->setCurrentIndex(settings.value("Background/Position", 0).toInt());
	connect(m_background_position, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBackground()));

	// Lay out tab
	tab_layout = new QGridLayout(tab);
	tab_layout->setMargin(12);
	tab_layout->setSpacing(6);
	tab_layout->setColumnStretch(0, 1);
	tab_layout->setColumnStretch(3, 1);
	tab_layout->setRowStretch(0, 1);
	tab_layout->setRowStretch(4, 1);
	tab_layout->addWidget(new QLabel(tr("Image:"), this), 1, 0, Qt::AlignRight | Qt::AlignBottom);
	tab_layout->addWidget(m_background_image, 1, 1);
	tab_layout->addWidget(new QLabel(tr("Style:"), this), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_background_position, 2, 1);
	tab_layout->addWidget(new QLabel(tr("Color:"), this), 3, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_background_color, 3, 1);


	// Create save tab
	tab = new QWidget(this);
	tabs->addTab(tab, tr("Saving"));

	m_location = new QPushButton(tab);
	m_location->setAutoDefault(false);
	m_location->setText(settings.value("Save/Location", QDir::currentPath()).toString());
	connect(m_location, SIGNAL(clicked()), this, SLOT(updateLocation()));

	m_auto_save = new QCheckBox(tr("Automatically save changes"), tab);
	m_auto_save->setCheckState(settings.value("Save/Auto", true).toBool() ? Qt::Checked : Qt::Unchecked);
	connect(m_auto_save, SIGNAL(stateChanged(int)), this, SLOT(updateAutoSave()));

	m_auto_append = new QCheckBox(tr("Automatically append filename extension"), tab);
	m_auto_append->setCheckState(settings.value("Save/Append", true).toBool() ? Qt::Checked : Qt::Unchecked);
	connect(m_auto_append, SIGNAL(stateChanged(int)), this, SLOT(updateAutoAppend()));

	// Lay out tab
	tab_layout = new QGridLayout(tab);
	tab_layout->setMargin(12);
	tab_layout->setSpacing(6);
	tab_layout->setColumnStretch(1, 1);
	tab_layout->setRowStretch(0, 1);
	tab_layout->setRowStretch(4, 1);
	tab_layout->addWidget(new QLabel(tr("Location:"), this), 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	tab_layout->addWidget(m_location, 1, 1);
	tab_layout->addWidget(m_auto_save, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
	tab_layout->addWidget(m_auto_append, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);
}

/*****************************************************************************/

void Preferences::emitSettings() {
	QColor page = m_page_color->color();
	page.setAlpha(m_page_opacity_slider->value() * 2.55f);
	emit colorsChanged(m_text_color->color(), page);
	emit backgroundChanged(m_background_color->color(), m_background_image->image(), m_background_position->currentIndex());
	emit widthChanged(m_page_width->value());
	emit fontChanged(QFont(m_font_names->currentFont().family(), m_font_sizes->currentText().toInt()));
	emit autoSaveChanged(m_auto_save->checkState() == Qt::Checked);
	emit autoAppendChanged(m_auto_append->checkState() == Qt::Checked);
}

/*****************************************************************************/

void Preferences::updateColors() {
	QColor page = m_page_color->color();
	page.setAlpha(m_page_opacity_slider->value() * 2.55f);
	QSettings settings;
	settings.setValue("Text/Color", m_text_color->toString());
	settings.setValue("Page/Color", page.name());
	settings.setValue("Page/Opacity", m_page_opacity_slider->value());
	emit colorsChanged(m_text_color->color(), page);
}

/*****************************************************************************/

void Preferences::updateBackground() {
	QSettings settings;
	settings.setValue("Background/Color", m_background_color->toString());
	settings.setValue("Background/Image", m_background_image->toString());
	settings.setValue("Background/Position", m_background_position->currentIndex());
	emit backgroundChanged(m_background_color->color(), m_background_image->image(), m_background_position->currentIndex());
}

/*****************************************************************************/

void Preferences::updateWidth() {
	int width = m_page_width->value();
	QSettings().setValue("Page/Width", width);
	emit widthChanged(width);
}

/*****************************************************************************/

void Preferences::updateFont() {
	QFont font(m_font_names->currentFont().family(), m_font_sizes->currentText().toInt());
	QSettings().setValue("Text/Font", font.toString());
	emit fontChanged(font);
}

/*****************************************************************************/

void Preferences::updateLocation() {
	QString location = QFileDialog::getExistingDirectory(this, tr("Find Directory"), m_location->text());
	if (!location.isEmpty()) {
		m_location->setText(location);
		QSettings().setValue("Save/Location", location);
		QDir::setCurrent(location);
	}
}

/*****************************************************************************/

void Preferences::updateAutoSave() {
	bool autosave = (m_auto_save->checkState() == Qt::Checked);
	QSettings().setValue("Save/Auto", autosave);
	emit autoSaveChanged(autosave);
}

/*****************************************************************************/

void Preferences::updateAutoAppend() {
	bool append = (m_auto_append->checkState() == Qt::Checked);
	QSettings().setValue("Save/Append", append);
	emit autoAppendChanged(append);
}

/*****************************************************************************/
