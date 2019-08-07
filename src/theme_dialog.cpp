/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "theme_dialog.h"

#include "color_button.h"
#include "font_combobox.h"
#include "image_button.h"
#include "theme.h"
#include "theme_renderer.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QDoubleValidator>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <cmath>

//-----------------------------------------------------------------------------

// Exported by QtGui
void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

//-----------------------------------------------------------------------------

ThemeDialog::ThemeDialog(Theme& theme, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_theme(theme)
{
	setWindowTitle(tr("Edit Theme"));
	setWindowModality(Qt::WindowModal);

	// Create name edit
	m_name = new QLineEdit(this);
	m_name->setText(m_theme.name());
	connect(m_name, &QLineEdit::textChanged, this, &ThemeDialog::checkNameAvailable);

	QHBoxLayout* name_layout = new QHBoxLayout;
	name_layout->setContentsMargins(0, 0, 0, 0);
	name_layout->addWidget(new QLabel(tr("Name:"), this));
	name_layout->addWidget(m_name);


	// Create scrollarea
	QWidget* contents = new QWidget(this);

	QScrollArea* scroll = new QScrollArea(this);
	scroll->setWidget(contents);
	scroll->setWidgetResizable(true);


	// Create text group
	QGroupBox* text_group = new QGroupBox(tr("Text"), contents);

	m_text_color = new ColorButton(text_group);
	m_text_color->setColor(m_theme.textColor());
	connect(m_text_color, &ColorButton::changed, this, &ThemeDialog::renderPreview);

	m_font_names = new FontComboBox(text_group);
	m_font_names->setEditable(false);
	m_font_names->setCurrentFont(m_theme.textFont());
	connect(m_font_names, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeDialog::fontChanged);
	connect(m_font_names, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeDialog::renderPreview);

	m_font_sizes = new QComboBox(text_group);
	m_font_sizes->setEditable(true);
	m_font_sizes->setMinimumContentsLength(3);
	connect(m_font_sizes, &QComboBox::editTextChanged, this, &ThemeDialog::renderPreview);
	fontChanged();

	m_misspelled_color = new ColorButton(text_group);
	m_misspelled_color->setColor(m_theme.misspelledColor());
	connect(m_misspelled_color, &ColorButton::changed, this, &ThemeDialog::renderPreview);

	QHBoxLayout* font_layout = new QHBoxLayout;
	font_layout->addWidget(m_font_names);
	font_layout->addWidget(m_font_sizes);

	QFormLayout* text_layout = new QFormLayout(text_group);
	text_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	text_layout->addRow(tr("Color:"), m_text_color);
	text_layout->addRow(tr("Font:"), font_layout);
	text_layout->addRow(tr("Misspelled:"), m_misspelled_color);


	// Create background group
	QGroupBox* background_group = new QGroupBox(tr("Window Background"), contents);

	m_background_color = new ColorButton(background_group);
	m_background_color->setColor(m_theme.backgroundColor());
	connect(m_background_color, &ColorButton::changed, this, &ThemeDialog::renderPreview);

	m_background_image = new ImageButton(background_group);
	m_background_image->setImage(m_theme.backgroundImage(), m_theme.backgroundPath());
	connect(m_background_image, &ImageButton::changed, this, &ThemeDialog::imageChanged);

	m_clear_image = new QPushButton(tr("Remove"), background_group);
	connect(m_clear_image, &QPushButton::clicked, m_background_image, &ImageButton::unsetImage);

	m_background_type = new QComboBox(background_group);
	m_background_type->addItems(QStringList() << tr("No Image") << tr("Tiled") << tr("Centered") << tr("Stretched") << tr("Scaled") << tr("Zoomed"));
	m_background_type->setCurrentIndex(m_theme.backgroundType());
	connect(m_background_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeDialog::renderPreview);

	QVBoxLayout* image_layout = new QVBoxLayout;
	image_layout->setSpacing(0);
	image_layout->setContentsMargins(0, 0, 0, 0);
	image_layout->addWidget(m_background_image);
	image_layout->addWidget(m_clear_image);

	QFormLayout* background_layout = new QFormLayout(background_group);
	background_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	background_layout->addRow(tr("Color:"), m_background_color);
	background_layout->addRow(tr("Image:"), image_layout);
	background_layout->addRow(tr("Type:"), m_background_type);


	// Create foreground group
	QGroupBox* foreground_group = new QGroupBox(tr("Text Background"), contents);

	m_foreground_color = new ColorButton(foreground_group);
	m_foreground_color->setColor(m_theme.foregroundColor());
	connect(m_foreground_color, &ColorButton::changed, this, &ThemeDialog::renderPreview);

	m_foreground_opacity = new QSpinBox(foreground_group);
	m_foreground_opacity->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_foreground_opacity->setSuffix(QLocale().percent());
	m_foreground_opacity->setRange(theme.foregroundOpacity().minimumValue(), theme.foregroundOpacity().maximumValue());
	m_foreground_opacity->setValue(m_theme.foregroundOpacity());
	connect(m_foreground_opacity, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	m_foreground_position = new QComboBox(foreground_group);
	m_foreground_position->addItems(QStringList() << tr("Left") << tr("Centered") << tr("Right") << tr("Stretched"));
	m_foreground_position->setCurrentIndex(m_theme.foregroundPosition());
	connect(m_foreground_position, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeDialog::positionChanged);

	m_foreground_width = new QSpinBox(foreground_group);
	m_foreground_width->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_foreground_width->setSuffix(tr(" pixels"));
	m_foreground_width->setRange(theme.foregroundWidth().minimumValue(), theme.foregroundWidth().maximumValue());
	m_foreground_width->setValue(m_theme.foregroundWidth());
	m_foreground_width->setEnabled(m_theme.foregroundPosition() != 3);
	connect(m_foreground_width, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	QFormLayout* foreground_layout = new QFormLayout(foreground_group);
	foreground_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	foreground_layout->addRow(tr("Color:"), m_foreground_color);
	foreground_layout->addRow(tr("Opacity:"), m_foreground_opacity);
	foreground_layout->addRow(tr("Position:"), m_foreground_position);
	foreground_layout->addRow(tr("Width:"), m_foreground_width);


	// Create rounding group
	m_round_corners = new QGroupBox(tr("Round Text Background Corners"), contents);
	m_round_corners->setCheckable(true);
	m_round_corners->setChecked(m_theme.roundCornersEnabled());
	connect(m_round_corners, &QGroupBox::clicked, this, &ThemeDialog::renderPreview);

	m_corner_radius = new QSpinBox(m_round_corners);
	m_corner_radius->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_corner_radius->setSuffix(tr(" pixels"));
	m_corner_radius->setRange(theme.cornerRadius().minimumValue(), theme.cornerRadius().maximumValue());
	m_corner_radius->setValue(m_theme.cornerRadius());
	connect(m_corner_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	QFormLayout* corner_layout = new QFormLayout(m_round_corners);
	corner_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	corner_layout->addRow(tr("Radius:"), m_corner_radius);


	// Create blur group
	m_blur = new QGroupBox(tr("Blur Text Background"), contents);
	m_blur->setCheckable(true);
	m_blur->setChecked(m_theme.blurEnabled());
	connect(m_blur, &QGroupBox::clicked, this, &ThemeDialog::renderPreview);

	m_blur_radius = new QSpinBox(m_blur);
	m_blur_radius->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_blur_radius->setSuffix(tr(" pixels"));
	m_blur_radius->setRange(theme.blurRadius().minimumValue(), theme.blurRadius().maximumValue());
	m_blur_radius->setValue(m_theme.blurRadius());
	connect(m_blur_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	QFormLayout* blur_layout = new QFormLayout(m_blur);
	blur_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	blur_layout->addRow(tr("Radius:"), m_blur_radius);


	// Create shadow group
	m_shadow = new QGroupBox(tr("Text Background Drop Shadow"), contents);
	m_shadow->setCheckable(true);
	m_shadow->setChecked(m_theme.shadowEnabled());
	connect(m_shadow, &QGroupBox::clicked, this, &ThemeDialog::renderPreview);

	m_shadow_color = new ColorButton(m_shadow);
	m_shadow_color->setColor(m_theme.shadowColor());
	connect(m_shadow_color, &ColorButton::changed, this, &ThemeDialog::renderPreview);

	m_shadow_radius = new QSpinBox(m_shadow);
	m_shadow_radius->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_shadow_radius->setSuffix(tr(" pixels"));
	m_shadow_radius->setRange(theme.shadowRadius().minimumValue(), theme.shadowRadius().maximumValue());
	m_shadow_radius->setValue(m_theme.shadowRadius());
	connect(m_shadow_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	m_shadow_offset = new QSpinBox(m_shadow);
	m_shadow_offset->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_shadow_offset->setSuffix(tr(" pixels"));
	m_shadow_offset->setRange(theme.shadowOffset().minimumValue(), theme.shadowOffset().maximumValue());
	m_shadow_offset->setValue(m_theme.shadowOffset());
	connect(m_shadow_offset, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	QFormLayout* shadow_layout = new QFormLayout(m_shadow);
	shadow_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	shadow_layout->addRow(tr("Color:"), m_shadow_color);
	shadow_layout->addRow(tr("Radius:"), m_shadow_radius);
	shadow_layout->addRow(tr("Vertical Offset:"), m_shadow_offset);


	// Create margins group
	QGroupBox* margins_group = new QGroupBox(tr("Margins"), contents);

	m_foreground_margin = new QSpinBox(margins_group);
	m_foreground_margin->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_foreground_margin->setSuffix(tr(" pixels"));
	m_foreground_margin->setRange(theme.foregroundMargin().minimumValue(), theme.foregroundMargin().maximumValue());
	m_foreground_margin->setValue(m_theme.foregroundMargin());
	connect(m_foreground_margin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	m_foreground_padding = new QSpinBox(margins_group);
	m_foreground_padding->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_foreground_padding->setSuffix(tr(" pixels"));
	m_foreground_padding->setRange(theme.foregroundPadding().minimumValue(), theme.foregroundPadding().maximumValue());
	m_foreground_padding->setValue(m_theme.foregroundPadding());
	connect(m_foreground_padding, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	QFormLayout* margins_layout = new QFormLayout(margins_group);
	margins_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	margins_layout->addRow(tr("Window:"), m_foreground_margin);
	margins_layout->addRow(tr("Page:"), m_foreground_padding);


	// Create line spacing group
	QGroupBox* line_spacing = new QGroupBox(tr("Line Spacing"), contents);

	m_line_spacing_type = new QComboBox(line_spacing);
	m_line_spacing_type->setEditable(false);
	m_line_spacing_type->addItems(QStringList() << tr("Single") << tr("1.5 Lines") << tr("Double") << tr("Proportional"));
	m_line_spacing_type->setCurrentIndex(3);

	m_line_spacing = new QSpinBox(line_spacing);
	m_line_spacing->setSuffix(QLocale().percent());
	m_line_spacing->setRange(theme.lineSpacing().minimumValue(), theme.lineSpacing().maximumValue());
	m_line_spacing->setValue(m_theme.lineSpacing());
	m_line_spacing->setEnabled(false);

	switch (m_theme.lineSpacing()) {
	case 100: m_line_spacing_type->setCurrentIndex(0); break;
	case 150: m_line_spacing_type->setCurrentIndex(1); break;
	case 200: m_line_spacing_type->setCurrentIndex(2); break;
	default: m_line_spacing->setEnabled(true); break;
	}
	connect(m_line_spacing_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeDialog::lineSpacingChanged);
	connect(m_line_spacing_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeDialog::renderPreview);
	connect(m_line_spacing, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	QFormLayout* line_spacing_layout = new QFormLayout(line_spacing);
	line_spacing_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	line_spacing_layout->addRow(tr("Type:"), m_line_spacing_type);
	line_spacing_layout->addRow(tr("Height:"), m_line_spacing);


	// Create paragraph spacing group
	QGroupBox* paragraph_spacing = new QGroupBox(tr("Paragraph Spacing"), contents);

	m_tab_width = new QSpinBox(paragraph_spacing);
	m_tab_width->setSuffix(tr(" pixels"));
	m_tab_width->setRange(theme.tabWidth().minimumValue(), theme.tabWidth().maximumValue());
	m_tab_width->setValue(m_theme.tabWidth());
	connect(m_tab_width, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	m_spacing_above_paragraph = new QSpinBox(paragraph_spacing);
	m_spacing_above_paragraph->setSuffix(tr(" pixels"));
	m_spacing_above_paragraph->setRange(theme.spacingAboveParagraph().minimumValue(), theme.spacingAboveParagraph().maximumValue());
	m_spacing_above_paragraph->setValue(m_theme.spacingAboveParagraph());
	connect(m_spacing_above_paragraph, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	m_spacing_below_paragraph = new QSpinBox(paragraph_spacing);
	m_spacing_below_paragraph->setSuffix(tr(" pixels"));
	m_spacing_below_paragraph->setRange(theme.spacingBelowParagraph().minimumValue(), theme.spacingBelowParagraph().maximumValue());
	m_spacing_below_paragraph->setValue(m_theme.spacingBelowParagraph());
	connect(m_spacing_below_paragraph, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeDialog::renderPreview);

	m_indent_first_line = new QCheckBox(tr("Indent first line"), paragraph_spacing);
	m_indent_first_line->setChecked(m_theme.indentFirstLine());
	connect(m_indent_first_line, &QCheckBox::toggled, this, &ThemeDialog::renderPreview);

	QFormLayout* paragraph_spacing_layout = new QFormLayout(paragraph_spacing);
	paragraph_spacing_layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
	paragraph_spacing_layout->addRow(tr("Tab Width:"), m_tab_width);
	paragraph_spacing_layout->addRow(tr("Above:"), m_spacing_above_paragraph);
	paragraph_spacing_layout->addRow(tr("Below:"), m_spacing_below_paragraph);
	paragraph_spacing_layout->addRow("", m_indent_first_line);


	// Create preview
	m_preview = new QLabel(this);
	m_preview->setAlignment(Qt::AlignCenter);
	m_preview->setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);

	QPixmap pixmap(480, 270);
	pixmap.fill(palette().window().color());
	m_preview->setPixmap(pixmap);

	m_theme_renderer = new ThemeRenderer(this);
	connect(m_theme_renderer, &ThemeRenderer::rendered, this, &ThemeDialog::renderText);
	renderPreview();


	// Lay out dialog
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_ok = buttons->button(QDialogButtonBox::Ok);
	connect(buttons, &QDialogButtonBox::accepted, this, &ThemeDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &ThemeDialog::reject);

	QVBoxLayout* groups_layout = new QVBoxLayout(contents);
	groups_layout->addWidget(text_group);
	groups_layout->addWidget(background_group);
	groups_layout->addWidget(foreground_group);
	groups_layout->addWidget(m_round_corners);
	groups_layout->addWidget(m_blur);
	groups_layout->addWidget(m_shadow);
	groups_layout->addWidget(margins_group);
	groups_layout->addWidget(line_spacing);
	groups_layout->addWidget(paragraph_spacing);

	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnStretch(0, 1);
	layout->setRowStretch(1, 1);
	layout->setRowMinimumHeight(2, layout->contentsMargins().top());
	layout->addLayout(name_layout, 0, 0, 1, 2);
	layout->addWidget(scroll, 1, 0, 1, 1);
	layout->addWidget(m_preview, 1, 1, 1, 1, Qt::AlignCenter);
	layout->addWidget(buttons, 3, 0, 1, 2);

	resize(QSettings().value("ThemeDialog/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

ThemeDialog::~ThemeDialog()
{
	m_theme_renderer->wait();
}

//-----------------------------------------------------------------------------

void ThemeDialog::accept()
{
	m_theme.setName(m_name->text().simplified());
	setValues(m_theme);
	if (!m_theme.isDefault()) {
		m_theme.setLoadColor(m_load_color);
	}
	m_theme.saveChanges();

	savePreview();

	QDialog::accept();
}

//-----------------------------------------------------------------------------

void ThemeDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("ThemeDialog/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void ThemeDialog::checkNameAvailable()
{
	QString name = m_name->text().simplified();
	bool empty = name.isEmpty();
	bool changed = (name != m_theme.name());
	bool exists = Theme::exists(name);
	m_ok->setEnabled(!changed || (!empty && !exists));
}

//-----------------------------------------------------------------------------

void ThemeDialog::fontChanged()
{
	QFontDatabase db;

	QFont font = m_font_names->currentFont();
	QList<int> font_sizes = db.smoothSizes(font.family(), QString());
	if (font_sizes.isEmpty()) {
		font_sizes = db.standardSizes();
	}
	qreal font_size = m_font_sizes->currentText().toDouble();
	if (font_size < 0.1) {
		font_size = std::lround(m_theme.textFont().pointSizeF() * 10.0) * 0.1;
	}

	m_font_sizes->blockSignals(true);
	m_font_sizes->clear();
	int index = 0;
	for (int i = 0; i < font_sizes.count(); ++i) {
		int size = font_sizes.at(i);
		if (size <= font_size) {
			index = i;
		}
		m_font_sizes->addItem(QString::number(size));
	}
	m_font_sizes->setCurrentIndex(index);
	m_font_sizes->setEditText(QString::number(font_size));
	m_font_sizes->setValidator(new QDoubleValidator(font_sizes.first(), font_sizes.last(), 1, m_font_sizes));
	m_font_sizes->blockSignals(false);
}

//-----------------------------------------------------------------------------

void ThemeDialog::imageChanged()
{
	if (!m_background_image->image().isEmpty()) {
		QSize image = QImageReader(m_background_image->image()).size();
		QSize desktop = QApplication::desktop()->size();
		if ((image.width() * image.height() * 4) <= (desktop.width() * desktop.height())) {
			m_background_type->setCurrentIndex(1);
		} else if (m_background_type->currentIndex() < 2) {
			m_background_type->setCurrentIndex(5);
		}
	} else {
		m_background_type->setCurrentIndex(0);
	}
	renderPreview();
}

//-----------------------------------------------------------------------------

void ThemeDialog::lineSpacingChanged(int index)
{
	switch (index) {
	case 0:
		m_line_spacing->setValue(100);
		m_line_spacing->setEnabled(false);
		break;

	case 1:
		m_line_spacing->setValue(150);
		m_line_spacing->setEnabled(false);
		break;

	case 2:
		m_line_spacing->setValue(200);
		m_line_spacing->setEnabled(false);
		break;

	default:
		m_line_spacing->setEnabled(true);
		break;
	}
}

//-----------------------------------------------------------------------------

void ThemeDialog::positionChanged(int index)
{
	m_foreground_width->setEnabled(index != 3);
	renderPreview();
}

//-----------------------------------------------------------------------------

void ThemeDialog::renderPreview()
{
	m_clear_image->setEnabled(m_background_image->isEnabled() && !m_background_image->image().isEmpty());

	// Load theme
	Theme theme;
	setValues(theme);
	theme.setBackgroundImage(m_background_image->image());

	// Fetch load color
	m_load_color = theme.calculateLoadColor();

	// Render theme
	m_theme_renderer->create(theme, QSize(1920, 1080), 0, devicePixelRatioF());
}

//-----------------------------------------------------------------------------

void ThemeDialog::renderText(const QImage& background, const QRect& foreground, const Theme& theme)
{
	QImage preview;
	theme.renderText(background, foreground, devicePixelRatioF(), &preview, &m_preview_icon);
	m_preview->setPixmap(QPixmap::fromImage(preview));
}

//-----------------------------------------------------------------------------

void ThemeDialog::savePreview()
{
	Theme::removeIcon(m_theme.id(), m_theme.isDefault());
	m_preview_icon.save(Theme::iconPath(m_theme.id(), m_theme.isDefault(), devicePixelRatioF()), "", 0);
}

//-----------------------------------------------------------------------------

void ThemeDialog::setValues(Theme& theme)
{
	theme.setBackgroundType(m_background_type->currentIndex());
	theme.setBackgroundColor(m_background_color->color());
	theme.setBackgroundImage(m_background_image->toString());

	theme.setForegroundColor(m_foreground_color->color());
	theme.setForegroundOpacity(m_foreground_opacity->value());
	theme.setForegroundWidth(m_foreground_width->value());
	theme.setForegroundMargin(m_foreground_margin->value());
	theme.setForegroundPadding(m_foreground_padding->value());
	theme.setForegroundPosition(m_foreground_position->currentIndex());

	theme.setRoundCornersEnabled(m_round_corners->isChecked());
	theme.setCornerRadius(m_corner_radius->value());

	theme.setBlurEnabled(m_blur->isChecked());
	theme.setBlurRadius(m_blur_radius->value());

	theme.setShadowEnabled(m_shadow->isChecked());
	theme.setShadowColor(m_shadow_color->color());
	theme.setShadowRadius(m_shadow_radius->value());
	theme.setShadowOffset(m_shadow_offset->value());

	theme.setTextColor(m_text_color->color());
	QFont font = m_font_names->currentFont();
	font.setPointSizeF(m_font_sizes->currentText().toDouble());
	theme.setTextFont(font);
	theme.setMisspelledColor(m_misspelled_color->color());

	theme.setIndentFirstLine(m_indent_first_line->isChecked());
	theme.setLineSpacing(m_line_spacing->value());
	theme.setSpacingAboveParagraph(m_spacing_above_paragraph->value());
	theme.setSpacingBelowParagraph(m_spacing_below_paragraph->value());
	theme.setTabWidth(m_tab_width->value());
}

//-----------------------------------------------------------------------------
