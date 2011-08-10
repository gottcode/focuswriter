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

#ifndef THEME_H
#define THEME_H

#include "settings_file.h"

#include <QColor>
#include <QCoreApplication>
#include <QFont>
class QImage;
class QSize;

class Theme : public SettingsFile
{
	Q_DECLARE_TR_FUNCTIONS(Theme)

public:
	Theme(const QString& name = QString());
	~Theme();

	static void copyBackgrounds();
	static QImage renderBackground(const QString& filename, int type, const QColor& background, const QSize& size);
	static QString path();
	static QString filePath(const QString& theme);
	static QString iconPath(const QString& theme);
	static void setPath(const QString& path);

	QString name() const;
	void setName(const QString& name);

	// Background settings
	int backgroundType() const;
	QColor backgroundColor() const;
	QString backgroundImage() const;
	QString backgroundPath() const;

	void setBackgroundType(int type);
	void setBackgroundColor(const QColor& color);
	void setBackgroundImage(const QString& path);

	// Foreground settings
	QColor foregroundColor() const;
	int foregroundOpacity() const;
	int foregroundWidth() const;
	int foregroundRounding() const;
	int foregroundMargin() const;
	int foregroundPadding() const;
	int foregroundPosition() const;

	void setForegroundColor(const QColor& color);
	void setForegroundOpacity(int opacity);
	void setForegroundWidth(int width);
	void setForegroundRounding(int rounding);
	void setForegroundMargin(int margin);
	void setForegroundPadding(int padding);
	void setForegroundPosition(int position);

	// Text settings
	QColor textColor() const;
	QFont textFont() const;
	QColor misspelledColor() const;
	QColor blurredTextColor() const;

	void setTextColor(const QColor& color);
	void setTextFont(const QFont& font);
	void setMisspelledColor(const QColor& color);
	void setBlurredTextColor(const QColor& color);

private:
	static QString m_path;
	QString m_name;

	int m_background_type;
	QColor m_background_color;
	QString m_background_path;
	QString m_background_image;

	QColor m_foreground_color;
	int m_foreground_opacity;
	int m_foreground_width;
	int m_foreground_rounding;
	int m_foreground_margin;
	int m_foreground_padding;
	int m_foreground_position;

	QColor m_text_color;
	QFont m_text_font;
	QColor m_misspelled_color;
	QColor m_blurred_text_color;
};

#endif
