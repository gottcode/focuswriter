/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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

#ifndef THEME_DIALOG_H
#define THEME_DIALOG_H

class ColorButton;
class FontComboBox;
class ImageButton;
class Theme;
class ThemeRenderer;

#include <QDialog>
#include <QFuture>
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class ThemeDialog : public QDialog
{
	Q_OBJECT

public:
	ThemeDialog(Theme& theme, QWidget* parent = 0);
	~ThemeDialog();


public slots:
	virtual void accept();

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void checkNameAvailable();
	void fontChanged();
	void imageChanged();
	void lineSpacingChanged(int index);
	void positionChanged(int index);
	void renderPreview();
	void renderText(const QImage& background, const QRect& foreground, const Theme& theme);

private:
	void savePreview();
	void setValues(Theme& theme);

private:
	Theme& m_theme;

	QLineEdit* m_name;
	QPushButton* m_ok;

	ThemeRenderer* m_theme_renderer;
	QLabel* m_preview;
	QImage m_preview_icon;
	QFuture<QColor> m_load_color;

	ColorButton* m_text_color;
	FontComboBox* m_font_names;
	QComboBox* m_font_sizes;
	ColorButton* m_misspelled_color;

	ColorButton* m_background_color;
	ImageButton* m_background_image;
	QPushButton* m_clear_image;
	QComboBox* m_background_type;

	ColorButton* m_foreground_color;
	QSpinBox* m_foreground_opacity;
	QComboBox* m_foreground_position;
	QSpinBox* m_foreground_width;

	QGroupBox* m_round_corners;
	QSpinBox* m_corner_radius;

	QGroupBox* m_blur;
	QSpinBox* m_blur_radius;

	QGroupBox* m_shadow;
	ColorButton* m_shadow_color;
	QSpinBox* m_shadow_radius;
	QSpinBox* m_shadow_offset;

	QSpinBox* m_foreground_margin;
	QSpinBox* m_foreground_padding;

	QComboBox* m_line_spacing_type;
	QSpinBox* m_line_spacing;

	QSpinBox* m_tab_width;
	QSpinBox* m_spacing_above_paragraph;
	QSpinBox* m_spacing_below_paragraph;
	QCheckBox* m_indent_first_line;
};

#endif
