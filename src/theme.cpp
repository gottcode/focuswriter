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

#include "theme.h"

#include "session.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QPainter>
#include <QSettings>
#include <QUrl>

//-----------------------------------------------------------------------------

bool compareFiles(const QString& filename1, const QString& filename2)
{
	// Compare sizes
	QFile file1(filename1);
	QFile file2(filename2);
	if (file1.size() != file2.size()) {
		return false;
	}

	// Compare contents
	bool equal = true;
	if (file1.open(QFile::ReadOnly) && file2.open(QFile::ReadOnly)) {
		while (!file1.atEnd()) {
			if (file1.read(1000) != file2.read(1000)) {
				equal = false;
				break;
			}
		}
		file1.close();
		file2.close();
	} else {
		equal = false;
	}
	return equal;
}

namespace
{
	QString copyImage(const QString& image)
	{
		// Check if already copied
		QDir images(Theme::path() + "/Images/");
		QStringList filenames = images.entryList(QDir::Files);
		foreach (const QString& filename, filenames) {
			if (compareFiles(image, images.filePath(filename))) {
				return filename;
			}
		}

		// Find file name
		QString base = QCryptographicHash::hash(image.toUtf8(), QCryptographicHash::Sha1).toHex();
		QString suffix = QFileInfo(image).suffix().toLower();
		QString filename = QString("%1.%2").arg(base, suffix);

		// Handle file name collisions
		int id = 0;
		while (images.exists(filename)) {
			id++;
			filename = QString("%1-%2.%3").arg(base).arg(id).arg(suffix);
		}

		QFile::copy(image, images.filePath(filename));
		return filename;
	}
}

//-----------------------------------------------------------------------------

QString Theme::m_path;

//-----------------------------------------------------------------------------

Theme::ThemeData::ThemeData(const QString& name_, bool create) :
	name(name_),
	background_type(0, 5),
	foreground_opacity(0, 100),
	foreground_width(500, 9999),
	foreground_rounding(0, 100),
	foreground_margin(1, 250),
	foreground_padding(0, 250),
	foreground_position(0, 3),
	line_spacing(50, 1000),
	paragraph_spacing_above(0, 1000),
	paragraph_spacing_below(0, 1000),
	tab_width(1, 1000)
{
	if (name.isEmpty() && create) {
		QString untitled;
		int count = 0;
		do {
			count++;
			untitled = Theme::tr("Untitled %1").arg(count);
		} while (QFile::exists(Theme::filePath(untitled)));
		name = untitled;
	}
}

//-----------------------------------------------------------------------------

Theme::Theme(const QString& name, bool create)
{
	d = new ThemeData(name, create);
	forgetChanges();
}

//-----------------------------------------------------------------------------

Theme::~Theme()
{
	saveChanges();
}

//-----------------------------------------------------------------------------

void Theme::copyBackgrounds()
{
	QDir dir(path() + "/Images");
	QStringList images;

	// Copy images
	QStringList themes = QDir(path(), "*.theme").entryList(QDir::Files);
	foreach (const QString& theme, themes) {
		QSettings settings(path() + "/" + theme, QSettings::IniFormat);
		QString background_path = settings.value("Background/Image").toString();
		QString background_image = settings.value("Background/ImageFile").toString();
		if (background_path.isEmpty() && background_image.isEmpty()) {
			continue;
		}
		if (!background_path.isEmpty() && (background_image.isEmpty() || !dir.exists(background_image))) {
			background_image = copyImage(background_path);
			settings.setValue("Background/ImageFile", background_image);
		}
		images.append(background_image);
	}

	// Delete unused images
	QStringList files = dir.entryList(QDir::Files);
	foreach (const QString& file, files) {
		if (!images.contains(file)) {
			QFile::remove(path() + "/Images/" + file);
		}
	}
}

//-----------------------------------------------------------------------------

QImage Theme::renderBackground(const QString& filename, int type, const QColor& background, const QSize& size)
{
	QImage image(size, QImage::Format_RGB32);
	image.fill(background.rgb());

	QPainter painter(&image);
	if (type > 1) {
		QImageReader source(filename);
		QSize scaled = source.size();
		switch (type) {
		case 3:
			scaled.scale(size, Qt::IgnoreAspectRatio);
			break;
		case 4:
			scaled.scale(size, Qt::KeepAspectRatio);
			break;
		case 5:
			scaled.scale(size, Qt::KeepAspectRatioByExpanding);
			break;
		default:
			break;
		}
		source.setScaledSize(scaled);
		painter.drawImage((size.width() - scaled.width()) / 2, (size.height() - scaled.height()) / 2, source.read());
	} else if (type == 1) {
		painter.fillRect(image.rect(), QImage(filename));
	}
	painter.end();
	return image;
}

//-----------------------------------------------------------------------------

QString Theme::filePath(const QString& theme)
{
	return m_path + "/" + QUrl::toPercentEncoding(theme, " ") + ".theme";
}

//-----------------------------------------------------------------------------

QString Theme::iconPath(const QString& theme)
{
	return m_path + "/" + QUrl::toPercentEncoding(theme, " ") + ".png";
}

//-----------------------------------------------------------------------------

void Theme::setName(const QString& name)
{
	if (d->name != name) {
		QStringList files = QDir(Session::path(), "*.session").entryList(QDir::Files);
		files.prepend("");
		foreach (const QString& file, files) {
			Session session(file);
			if (session.theme() == d->name) {
				session.setTheme(name);
			}
		}

		QFile::remove(filePath(d->name));
		QFile::remove(iconPath(d->name));
		setValue(d->name, name);
	}
}

//-----------------------------------------------------------------------------

void Theme::setBackgroundImage(const QString& path)
{
	if (d->background_path != path) {
		setValue(d->background_path, path);
		if (!d->background_path.isEmpty()) {
			d->background_image = copyImage(d->background_path);
		} else {
			d->background_image.clear();
		}
	}
}

//-----------------------------------------------------------------------------

bool Theme::operator==(const Theme& theme) const
{
	return (d->name == theme.d->name)

		&& (d->background_type == theme.d->background_type)
		&& (d->background_color == theme.d->background_color)
		&& (d->background_path == theme.d->background_path)
		&& (d->background_image == theme.d->background_image)

		&& (d->foreground_color == theme.d->foreground_color)
		&& (d->foreground_opacity == theme.d->foreground_opacity)
		&& (d->foreground_width == theme.d->foreground_width)
		&& (d->foreground_rounding == theme.d->foreground_rounding)
		&& (d->foreground_margin == theme.d->foreground_margin)
		&& (d->foreground_padding == theme.d->foreground_padding)
		&& (d->foreground_position == theme.d->foreground_position)

		&& (d->text_color == theme.d->text_color)
		&& (d->text_font == theme.d->text_font)
		&& (d->misspelled_color == theme.d->misspelled_color)

		&& (d->indent_first_line == theme.d->indent_first_line)
		&& (d->line_spacing == theme.d->line_spacing)
		&& (d->paragraph_spacing_above == theme.d->paragraph_spacing_above)
		&& (d->paragraph_spacing_below == theme.d->paragraph_spacing_below)
		&& (d->tab_width == theme.d->tab_width);
}

//-----------------------------------------------------------------------------

void Theme::reload()
{
	if (d->name.isEmpty()) {
		return;
	}

	QSettings settings(filePath(d->name), QSettings::IniFormat);

	// Load background settings
	d->background_type = settings.value("Background/Type", 0).toInt();
	d->background_color = settings.value("Background/Color", "#cccccc").toString();
	d->background_path = settings.value("Background/Image").toString();
	d->background_image = settings.value("Background/ImageFile").toString();
	if (!d->background_path.isEmpty() && d->background_image.isEmpty()) {
		setValue(d->background_image, copyImage(d->background_path));
	}

	// Load foreground settings
	d->foreground_color = settings.value("Foreground/Color", "#cccccc").toString();
	d->foreground_opacity = settings.value("Foreground/Opacity", 100).toInt();
	d->foreground_width = settings.value("Foreground/Width", 700).toInt();
	d->foreground_rounding = settings.value("Foreground/Rounding", 0).toInt();
	d->foreground_margin = settings.value("Foreground/Margin", 65).toInt();
	d->foreground_padding = settings.value("Foreground/Padding", 0).toInt();
	d->foreground_position = settings.value("Foreground/Position", 1).toInt();

	// Load text settings
	d->text_color = settings.value("Text/Color", "#000000").toString();
	d->text_font.fromString(settings.value("Text/Font", QFont("Times New Roman").toString()).toString());
	d->misspelled_color = settings.value("Text/Misspelled", "#ff0000").toString();

	// Load spacings
	d->indent_first_line = settings.value("Spacings/IndentFirstLine", false).toBool();
	d->line_spacing = settings.value("Spacings/LineSpacing", 100).toInt();
	d->paragraph_spacing_above = settings.value("Spacings/ParagraphAbove", 0).toInt();
	d->paragraph_spacing_below = settings.value("Spacings/ParagraphBelow", 0).toInt();
	d->tab_width = settings.value("Spacings/TabWidth", 48).toInt();
}

//-----------------------------------------------------------------------------

void Theme::write()
{
	if (d->name.isEmpty()) {
		return;
	}

	QSettings settings(filePath(d->name), QSettings::IniFormat);

	// Store background settings
	settings.setValue("Background/Type", d->background_type.value());
	settings.setValue("Background/Color", d->background_color.name());
	if (!d->background_path.isEmpty()) {
		settings.setValue("Background/Image", d->background_path);
	}
	settings.setValue("Background/ImageFile", d->background_image);

	// Store foreground settings
	settings.setValue("Foreground/Color", d->foreground_color.name());
	settings.setValue("Foreground/Opacity", d->foreground_opacity.value());
	settings.setValue("Foreground/Width", d->foreground_width.value());
	settings.setValue("Foreground/Rounding", d->foreground_rounding.value());
	settings.setValue("Foreground/Margin", d->foreground_margin.value());
	settings.setValue("Foreground/Padding", d->foreground_padding.value());
	settings.setValue("Foreground/Position", d->foreground_position.value());

	// Store text settings
	settings.setValue("Text/Color", d->text_color.name());
	settings.setValue("Text/Font", d->text_font.toString());
	settings.setValue("Text/Misspelled", d->misspelled_color.name());

	// Store spacings
	settings.setValue("Spacings/IndentFirstLine", d->indent_first_line);
	settings.setValue("Spacings/LineSpacing", d->line_spacing.value());
	settings.setValue("Spacings/ParagraphAbove", d->paragraph_spacing_above.value());
	settings.setValue("Spacings/ParagraphBelow", d->paragraph_spacing_below.value());
	settings.setValue("Spacings/TabWidth", d->tab_width.value());
}

//-----------------------------------------------------------------------------
