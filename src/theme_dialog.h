/***********************************************************************
 *
 * Copyright (C) 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include <QDialog>
class QComboBox;
class QFontComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class ColorButton;
class ImageButton;
class Theme;

class ThemeDialog : public QDialog {
	Q_OBJECT
public:
	ThemeDialog(Theme& theme, QWidget* parent = 0);

	static void createPreview(const QString& name);

public slots:
	virtual void accept();

private slots:
	void checkNameAvailable();
	void renderPreview();

private:
	void savePreview();

private:
	Theme& m_theme;

	QLineEdit* m_name;
	QPushButton* m_ok;

	QLabel* m_preview;

	QComboBox* m_background_type;
	ColorButton* m_background_color;
	ImageButton* m_background_image;

	ColorButton* m_foreground_color;
	QSpinBox* m_foreground_width;
	QSpinBox* m_foreground_margin;
	QSpinBox* m_foreground_padding;
	QSpinBox* m_foreground_opacity;
	QComboBox* m_foreground_position;

	ColorButton* m_text_color;
	QFontComboBox* m_font_names;
	QComboBox* m_font_sizes;
	ColorButton* m_misspelled_color;
};

#endif
