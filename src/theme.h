/*
	SPDX-FileCopyrightText: 2009-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_THEME_H
#define FOCUSWRITER_THEME_H

#include "ranged_int.h"
#include "settings_file.h"

#include <QColor>
#include <QCoreApplication>
#include <QExplicitlySharedDataPointer>
#include <QFont>
#include <QFuture>
#include <QSharedData>
class QImage;
class QSize;

class Theme : public SettingsFile
{
	Q_DECLARE_TR_FUNCTIONS(Theme)

	class ThemeData : public QSharedData
	{
	public:
		ThemeData(const QString& id, bool is_default, bool create);

		QString id;
		QString name;
		bool is_default;

		QColor load_color;

		RangedInt background_type;
		QColor background_color;
		QString background_path;
		QString background_image;

		QColor foreground_color;
		RangedInt foreground_opacity;
		RangedInt foreground_width;
		RangedInt foreground_margin;
		RangedInt foreground_padding;
		RangedInt foreground_position;

		bool round_corners_enabled;
		RangedInt corner_radius;

		bool blur_enabled;
		RangedInt blur_radius;

		bool shadow_enabled;
		RangedInt shadow_offset;
		RangedInt shadow_radius;
		QColor shadow_color;

		QColor text_color;
		QFont text_font;
		QColor misspelled_color;

		bool indent_first_line;
		RangedInt line_spacing;
		RangedInt paragraph_spacing_above;
		RangedInt paragraph_spacing_below;
		RangedInt tab_width;
	};
	QSharedDataPointer<ThemeData> d;

public:
	explicit Theme();
	Theme(const Theme& theme);
	Theme(const QString& id, bool is_default);
	~Theme();
	Theme& operator=(const Theme& theme);

	static QString clone(const QString& id, bool is_default, const QString& name);
	static void copyBackgrounds();
	static QString createId();
	static QString defaultId() { return "writingdesk"; }
	static bool exists(const QString& name);
	static QString filePath(const QString& id, bool is_default = false);
	static QString iconPath(const QString& id, bool is_default, qreal pixelratio);
	static QString path() { return m_path; }
	static void removeIcon(const QString& id, bool is_default);
	static void setDefaultPath(const QString& path);
	static void setPath(const QString& path);

	QImage render(const QSize& background, QRect& foreground, const int margin, const qreal pixelratio) const;
	void renderText(QImage background, const QRect& foreground, const qreal pixelratio, QImage* preview, QImage* icon) const;

	// Name settings
	bool isDefault() const { return d->is_default; }
	QString id() const { return d->id; }
	QString name() const { return d->name; }
	void setName(const QString& name) { setValue(d->name, name); }

	QFuture<QColor> calculateLoadColor() const;
	QColor loadColor() const { return d->load_color; }
	void setLoadColor(const QColor& color) { setValue(d->load_color, color); }

	// Background settings
	int backgroundType() const { return d->background_type; }
	QColor backgroundColor() const { return d->background_color; }
	QString backgroundImage() const;
	QString backgroundPath() const { return d->background_path; }

	void setBackgroundType(int type) { setValue(d->background_type, type); }
	void setBackgroundColor(const QColor& color) { setValue(d->background_color, color); }
	void setBackgroundImage(const QString& path);

	// Foreground settings
	QColor foregroundColor() const { return d->foreground_color; }
	RangedInt foregroundOpacity() const { return d->foreground_opacity; }
	RangedInt foregroundWidth() const { return d->foreground_width; }
	RangedInt foregroundMargin() const { return d->foreground_margin; }
	RangedInt foregroundPadding() const { return d->foreground_padding; }
	RangedInt foregroundPosition() const { return d->foreground_position; }
	QRect foregroundRect(const QSize& size, int margin, const qreal pixelratio) const;

	void setForegroundColor(const QColor& color) { setValue(d->foreground_color, color); }
	void setForegroundOpacity(int opacity) { setValue(d->foreground_opacity, opacity); }
	void setForegroundWidth(int width) { setValue(d->foreground_width, width); }
	void setForegroundMargin(int margin) { setValue(d->foreground_margin, margin); }
	void setForegroundPadding(int padding) { setValue(d->foreground_padding, padding); }
	void setForegroundPosition(int position) { setValue(d->foreground_position, position); }

	bool roundCornersEnabled() const { return d->round_corners_enabled; }
	RangedInt cornerRadius() const { return d->corner_radius; }

	void setRoundCornersEnabled(bool enabled) { setValue(d->round_corners_enabled, enabled); }
	void setCornerRadius(int radius) { setValue(d->corner_radius, radius); }

	bool blurEnabled() const { return d->blur_enabled; }
	RangedInt blurRadius() const { return d->blur_radius; }

	void setBlurEnabled(bool enabled) { setValue(d->blur_enabled, enabled); }
	void setBlurRadius(int radius) { setValue(d->blur_radius, radius); }

	bool shadowEnabled() const { return d->shadow_enabled; }
	QColor shadowColor() const { return d->shadow_color; }
	RangedInt shadowRadius() const { return d->shadow_radius; }
	RangedInt shadowOffset() const { return d->shadow_offset; }

	void setShadowEnabled(bool enabled) { setValue(d->shadow_enabled, enabled); }
	void setShadowColor(const QColor& color) { setValue(d->shadow_color, color); }
	void setShadowRadius(int radius) { setValue(d->shadow_radius, radius); }
	void setShadowOffset(int offset) { setValue(d->shadow_offset, offset); }

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
	void reload() override;
	void write() override;

private:
	static QString m_path_default;
	static QString m_path;
};

Q_DECLARE_METATYPE(Theme)

#endif // FOCUSWRITER_THEME_H
