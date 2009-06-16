/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
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

#include <QFile>
#include <QSettings>

/*****************************************************************************/

QString Theme::m_path;

/*****************************************************************************/

Theme::Theme(const QString& name)
: m_name(name),
  m_changed(false) {
	if (m_name.isEmpty()) {
		int count = 0;
		do {
			count++;
			m_name = tr("Untitled %1").arg(count);
		} while (QFile::exists(filePath(m_name)));
	}
	QSettings settings(filePath(m_name), QSettings::IniFormat);

	// Load background settings
	m_background_type = settings.value("Background/Type", 0).toInt();
	m_background_color = settings.value("Background/Color", "#cccccc").toString();
	m_background_image = settings.value("Background/Image").toString();

	// Load foreground settings
	m_foreground_color = settings.value("Foreground/Color", "#cccccc").toString();
	m_foreground_width = settings.value("Foreground/Width", 700).toInt();
	m_foreground_opacity = settings.value("Foreground/Opacity", 100).toInt();

	// Load text settings
	m_text_color = settings.value("Text/Color", "#000000").toString();
	m_text_font.fromString(settings.value("Text/Font", QFont().toString()).toString());
	m_misspelled_color = settings.value("Text/Misspelled", "#ff0000").toString();
}

/*****************************************************************************/

Theme::~Theme() {
	if (!m_changed) {
		return;
	}

	QSettings settings(filePath(m_name), QSettings::IniFormat);

	// Store background settings
	settings.setValue("Background/Type", m_background_type);
	settings.setValue("Background/Color", m_background_color.name());
	settings.setValue("Background/Image", m_background_image);

	// Store foreground settings
	settings.setValue("Foreground/Color", m_foreground_color.name());
	settings.setValue("Foreground/Width", m_foreground_width);
	settings.setValue("Foreground/Opacity", m_foreground_opacity);

	// Store text settings
	settings.setValue("Text/Color", m_text_color.name());
	settings.setValue("Text/Font", m_text_font.toString());
	settings.setValue("Text/Misspelled", m_misspelled_color.name());
}

/*****************************************************************************/

QString Theme::path() {
	return m_path;
}

/*****************************************************************************/

QString Theme::filePath(const QString& theme) {
	return path() + "/" + theme + ".theme";
}

/*****************************************************************************/

QString Theme::iconPath(const QString& theme) {
	return path() + "/" + theme + ".png";
}

/*****************************************************************************/

QString Theme::backgroundPath(const QString& theme) {
	return path() + "/" + theme + "-background.png";
}

/*****************************************************************************/

void Theme::setPath(const QString& path) {
	m_path = path;
}

/*****************************************************************************/

QString Theme::name() const {
	return m_name;
}

/*****************************************************************************/

void Theme::setName(const QString& name) {
	QFile::remove(filePath(m_name));
	QFile::remove(iconPath(m_name));
	m_name = name;
	m_changed = true;
}

/*****************************************************************************/

int Theme::backgroundType() const {
	return m_background_type;
}

/*****************************************************************************/

QColor Theme::backgroundColor() const {
	return m_background_color;
}

/*****************************************************************************/

QString Theme::backgroundImage() const {
	return m_background_image;
}

/*****************************************************************************/

void Theme::setBackgroundType(int type) {
	m_background_type = type;
	m_changed = true;
}

/*****************************************************************************/

void Theme::setBackgroundColor(const QColor& color) {
	m_background_color = color;
	m_changed = true;
}

/*****************************************************************************/

void Theme::setBackgroundImage(const QString& path) {
	m_background_image = path;
	m_changed = true;
}

/*****************************************************************************/

QColor Theme::foregroundColor() const {
	return m_foreground_color;
}

/*****************************************************************************/

int Theme::foregroundWidth() const {
	return m_foreground_width;
}

/*****************************************************************************/

int Theme::foregroundOpacity() const {
	return m_foreground_opacity;
}

/*****************************************************************************/

void Theme::setForegroundColor(const QColor& color) {
	m_foreground_color = color;
	m_changed = true;
}

/*****************************************************************************/

void Theme::setForegroundWidth(int width) {
	m_foreground_width = width;
	m_changed = true;
}

/*****************************************************************************/

void Theme::setForegroundOpacity(int opacity) {
	m_foreground_opacity = opacity;
	m_changed = true;
}

/*****************************************************************************/

QColor Theme::textColor() const {
	return m_text_color;
}

/*****************************************************************************/

QFont Theme::textFont() const {
	return m_text_font;
}

/*****************************************************************************/

QColor Theme::misspelledColor() const {
	return m_misspelled_color;
}

/*****************************************************************************/

void Theme::setTextColor(const QColor& color) {
	m_text_color = color;
	m_changed = true;
}

/*****************************************************************************/

void Theme::setTextFont(const QFont& font) {
	m_text_font = font;
	m_changed = true;
}

/*****************************************************************************/

void Theme::setMisspelledColor(const QColor& color) {
	m_misspelled_color = color;
	m_changed = true;
}

/*****************************************************************************/
