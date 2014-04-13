/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#include <QDialog>
class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextEdit;

class ThemeDialog : public QDialog
{
	Q_OBJECT

public:
	ThemeDialog(Theme& theme, QWidget* parent = 0);
	~ThemeDialog();

	static void createPreview(const QString& name);

public slots:
	virtual void accept();

private slots:
	void checkNameAvailable();
	void fontChanged();
	void imageChanged();
	void lineSpacingChanged(int index);
	void renderPreview();

private:
	void savePreview();
	void setValues(Theme& theme);

private:
	Theme& m_theme;

	QLineEdit* m_name;
	QPushButton* m_ok;

	QLabel* m_preview;
	QLabel* m_preview_background;
	QTextEdit* m_preview_text;

	QComboBox* m_background_type;
	ColorButton* m_background_color;
	ImageButton* m_background_image;
	QPushButton* m_clear_image;

	ColorButton* m_foreground_color;
	QSpinBox* m_foreground_opacity;
	QSpinBox* m_foreground_width;
	QSpinBox* m_foreground_rounding;
	QSpinBox* m_foreground_margin;
	QSpinBox* m_foreground_padding;
	QComboBox* m_foreground_position;

	ColorButton* m_text_color;
	FontComboBox* m_font_names;
	QComboBox* m_font_sizes;
	ColorButton* m_misspelled_color;

	QCheckBox* m_indent_first_line;
	QComboBox* m_line_spacing_type;
	QSpinBox* m_line_spacing;
	QSpinBox* m_spacing_above_paragraph;
	QSpinBox* m_spacing_below_paragraph;
	QSpinBox* m_tab_width;
};

#endif
