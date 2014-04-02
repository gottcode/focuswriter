/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "ranged_int.h"
#include "settings_file.h"

#include <QColor>
#include <QCoreApplication>
#include <QExplicitlySharedDataPointer>
#include <QFont>
#include <QSharedData>
class QImage;
class QSize;

class Theme : public SettingsFile
{
	Q_DECLARE_TR_FUNCTIONS(Theme)

	class ThemeData : public QSharedData
	{
	public:
		ThemeData(const QString& name = QString(), bool create = true);

		QString name;

		RangedInt background_type;
		QColor background_color;
		QString background_path;
		QString background_image;

		QColor foreground_color;
		RangedInt foreground_opacity;
		RangedInt foreground_width;
		RangedInt foreground_rounding;
		RangedInt foreground_margin;
		RangedInt foreground_padding;
		RangedInt foreground_position;

		QColor text_color;
		QFont text_font;
		QColor misspelled_color;

		bool indent_first_line;
		RangedInt line_spacing;
		RangedInt paragraph_spacing_above;
		RangedInt paragraph_spacing_below;
		RangedInt tab_width;
	};
	QExplicitlySharedDataPointer<ThemeData> d;

public:
	Theme(const QString& name = QString(), bool create = true);
	~Theme();

	static void copyBackgrounds();
	static QString filePath(const QString& theme);
	static QString iconPath(const QString& theme);
	static QString path() { return m_path; }
	static void setPath(const QString& path) { m_path = path; }

	QImage renderBackground(const QSize& background) const;

	// Name settings
	QString name() const { return d->name; }
	void setName(const QString& name);

	// Background settings
	int backgroundType() const { return d->background_type; }
	QColor backgroundColor() const { return d->background_color; }
	QString backgroundImage() const { return m_path + "/Images/" + d->background_image; }
	QString backgroundPath() const { return d->background_path; }

	void setBackgroundType(int type) { setValue(d->background_type, type); }
	void setBackgroundColor(const QColor& color) { setValue(d->background_color, color); }
	void setBackgroundImage(const QString& path);

	// Foreground settings
	QColor foregroundColor() const { return d->foreground_color; }
	RangedInt foregroundOpacity() const { return d->foreground_opacity; }
	RangedInt foregroundWidth() const { return d->foreground_width; }
	RangedInt foregroundRounding() const { return d->foreground_rounding; }
	RangedInt foregroundMargin() const { return d->foreground_margin; }
	RangedInt foregroundPadding() const { return d->foreground_padding; }
	RangedInt foregroundPosition() const { return d->foreground_position; }

	void setForegroundColor(const QColor& color) { setValue(d->foreground_color, color); }
	void setForegroundOpacity(int opacity) { setValue(d->foreground_opacity, opacity); }
	void setForegroundWidth(int width) { setValue(d->foreground_width, width); }
	void setForegroundRounding(int rounding) { setValue(d->foreground_rounding, rounding); }
	void setForegroundMargin(int margin) { setValue(d->foreground_margin, margin); }
	void setForegroundPadding(int padding) { setValue(d->foreground_padding, padding); }
	void setForegroundPosition(int position) { setValue(d->foreground_position, position); }

	// Text settings
	QColor textColor() const { return d->text_color; }
	QFont textFont() const { return d->text_font; }
	QColor misspelledColor() const { return d->misspelled_color; }

	void setTextColor(const QColor& color) { setValue(d->text_color, color); }
	void setTextFont(const QFont& font) { setValue(d->text_font, font); }
	void setMisspelledColor(const QColor& color) { setValue(d->misspelled_color, color); }

	// Spacing settings
	bool indentFirstLine() const { return d->indent_first_line; }
	RangedInt lineSpacing() const { return d->line_spacing; }
	RangedInt spacingAboveParagraph() const { return d->paragraph_spacing_above; }
	RangedInt spacingBelowParagraph() const { return d->paragraph_spacing_below; }
	RangedInt tabWidth() const { return d->tab_width; }

	void setIndentFirstLine(bool indent) { setValue(d->indent_first_line, indent); }
	void setLineSpacing(int spacing) { setValue(d->line_spacing, spacing); }
	void setSpacingAboveParagraph(int spacing) { setValue(d->paragraph_spacing_above, spacing); }
	void setSpacingBelowParagraph(int spacing) { setValue(d->paragraph_spacing_below, spacing); }
	void setTabWidth(int width) { setValue(d->tab_width, width); }

	bool operator==(const Theme& theme) const;

private:
	void reload();
	void write();

private:
	static QString m_path;
};

#endif
