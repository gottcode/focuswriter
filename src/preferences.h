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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
class QCheckBox;
class QComboBox;
class QFontComboBox;
class QPushButton;
class QSlider;
class QSpinBox;
class ColorButton;
class ImageButton;

class Preferences : public QDialog {
	Q_OBJECT
public:
	Preferences(QWidget* parent = 0);

	void emitSettings();

signals:
	void colorsChanged(const QColor& text, const QColor& page);
	void backgroundChanged(const QColor& color, const QImage& image, int position);
	void widthChanged(int width);
	void fontChanged(const QFont& font);
	void autoSaveChanged(bool enabled);
	void autoAppendChanged(bool enabled);

private slots:
	void updateColors();
	void updateBackground();
	void updateWidth();
	void updateFont();
	void updateLocation();
	void updateAutoSave();
	void updateAutoAppend();

private:
	ColorButton* m_text_color;
	ColorButton* m_page_color;
	QSlider* m_page_opacity_slider;
	QSpinBox* m_page_opacity_spinbox;
	ColorButton* m_background_color;
	ImageButton* m_background_image;
	QComboBox* m_background_position;
	QSpinBox* m_page_width;
	QFontComboBox* m_font_names;
	QComboBox* m_font_sizes;
	QPushButton* m_location;
	QCheckBox* m_auto_save;
	QCheckBox* m_auto_append;
};

#endif
