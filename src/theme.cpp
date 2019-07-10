/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 Graeme Gott <graeme@gottcode.org>
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

#include "utils.h"

#include <QtConcurrentRun>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QPainter>
#include <QSettings>
#include <QTextEdit>
#include <QUuid>

#include <cmath>

//-----------------------------------------------------------------------------

// Exported by QtGui
void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

//-----------------------------------------------------------------------------

namespace
{
	QColor averageImage(const QString& filename, const QColor& fallback)
	{
		QImageReader reader(filename);
		if (!reader.canRead()) {
			return fallback;
		}

		QImage image(reader.size(), QImage::Format_ARGB32_Premultiplied);
		image.fill(fallback.rgb());
		{
			QPainter painter(&image);
			painter.drawImage(0, 0, reader.read());
		}
		const unsigned int width = image.width();
		const unsigned int height = image.height();

		quint64 sum_r = 0;
		quint64 sum_g = 0;
		quint64 sum_b = 0;
		quint64 sum_a = 0;

		for (unsigned int y = 0; y < height; ++y) {
			const QRgb* scanline = reinterpret_cast<const QRgb*>(image.scanLine(y));
			for (unsigned int x = 0; x < width; ++x) {
				QRgb pixel = scanline[x];
				sum_r += qRed(pixel);
				sum_g += qGreen(pixel);
				sum_b += qBlue(pixel);
				sum_a += qAlpha(pixel);
			}
		}

		const qreal divisor = 1.0 / (width * height);
		return QColor(sum_r * divisor, sum_g * divisor, sum_b * divisor, sum_a * divisor);
	}

	QString checksumName(const QString& image)
	{
		QCryptographicHash hash(QCryptographicHash::Sha1);
		QFile file(image);
		if (file.open(QFile::ReadOnly)) {
			hash.addData(&file);
			file.close();
		}

		const QString suffix = QFileInfo(image).suffix().toLower();

		return "2-" + hash.result().toHex() + "." + suffix;
	}

	QString copyImage(const QString& image)
	{
		const QString name = checksumName(image);
		const QString path = Theme::path() + "/Images/" + name;
		if (!QFile::exists(path)) {
			QFile::copy(image, path);
		}
		return name;
	}

	QDir listIcons(const QString& id, bool is_default)
	{
		const QString icon = Theme::iconPath(id, is_default, 1.0);

		const int dirindex = icon.lastIndexOf('/');
		const int baseindex = icon.lastIndexOf('.');
		const QString basename = icon.mid(dirindex + 1, baseindex - dirindex - 1);

		return QDir(icon.left(dirindex), basename + "*");
	}
}

//-----------------------------------------------------------------------------

QString Theme::m_path_default;
QString Theme::m_path;

//-----------------------------------------------------------------------------

Theme::ThemeData::ThemeData(const QString& theme_id, bool theme_default, bool create) :
	id(theme_id),
	is_default(theme_default),
	background_type(0, 5),
	foreground_opacity(0, 100),
	foreground_width(500, 9999),
	foreground_margin(1, 250),
	foreground_padding(0, 250),
	foreground_position(0, 3),
	round_corners_enabled(false),
	corner_radius(1, 100),
	blur_enabled(false),
	blur_radius(1, 128),
	shadow_enabled(false),
	shadow_offset(0, 128),
	shadow_radius(1, 128),
	line_spacing(50, 1000),
	paragraph_spacing_above(0, 1000),
	paragraph_spacing_below(0, 1000),
	tab_width(1, 1000)
{
	if (id.isEmpty() && create) {
		QString untitled;
		int count = 0;
		do {
			count++;
			untitled = Theme::tr("Untitled %1").arg(count);
		} while (exists(untitled));
		name = untitled;
		id = createId();
	}
}

//-----------------------------------------------------------------------------

Theme::Theme()
{
	d = new ThemeData(QString(), false, false);
}

//-----------------------------------------------------------------------------

Theme::Theme(const Theme& theme) :
	d(theme.d)
{
}

//-----------------------------------------------------------------------------

Theme::Theme(const QString& id, bool is_default)
{
	d = new ThemeData(id, is_default, true);
	forgetChanges();
}

//-----------------------------------------------------------------------------

Theme::~Theme()
{
}

//-----------------------------------------------------------------------------

Theme& Theme::operator=(const Theme& theme)
{
	d = theme.d;
	return *this;
}

//-----------------------------------------------------------------------------

QString Theme::clone(const QString& id, bool is_default, const QString& name)
{
	if (id.isEmpty()) {
		return id;
	}

	// Find name for duplicate theme
	QStringList values = splitStringAtLastNumber(name);
	int count = values.at(1).toInt();
	QString new_name;
	do {
		++count;
		new_name = values.at(0) + QString::number(count);
	} while (exists(new_name));

	// Create duplicate
	QString new_id = createId();
	{
		Theme duplicate(id, is_default);
		duplicate.setValue(duplicate.d->name, new_name);
		duplicate.setValue(duplicate.d->id, new_id);
		duplicate.saveChanges();
	}

	// Copy icon
	const QDir dir = listIcons(id, is_default);
	const int suffix = dir.nameFilters().first().length() -1;
	const QStringList files = dir.entryList();
	for (const QString& file : files) {
		QFile::copy(dir.filePath(file), dir.filePath(new_id + file.mid(suffix)));
	}

	return new_id;
}

//-----------------------------------------------------------------------------

void Theme::copyBackgrounds()
{
	QDir dir(path() + "/Images");
	QStringList images;
	QHash<QString, QString> old_images;
	const QHash<QString, QString> source_images = {
		{ "2-77534bf3da7fb42c830772be8d279be79869deb7.jpg", m_path_default + "/images/spacedreams.jpg" },
		{ "2-1ccf9867f755b306830852e8fbf36952f93ab3fe.jpg", m_path_default + "/images/writingdesk.jpg" }
	};

	// Copy images
	const QStringList themes = QDir(path(), "*.theme").entryList(QDir::Files);
	for (const QString& theme : themes) {
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

		// Set image filename to checksum of image contents
		if (!background_image.startsWith("2-")) {
			if (!old_images.contains(background_image)) {
				const QString file = checksumName(dir.filePath(background_image));
				old_images.insert(background_image, file);
				dir.rename(background_image, file);
			}
			background_image = old_images[background_image];
			settings.setValue("Background/ImageFile", background_image);
		}

		// Replace lower resolution copies of default images
		if (source_images.contains(background_image)) {
			background_image = copyImage(source_images[background_image]);
			settings.setValue("Background/ImageFile", background_image);
		}

		images.append(background_image);
	}

	// Delete unused images
	const QStringList files = dir.entryList(QDir::Files);
	for (const QString& file : files) {
		if (!images.contains(file)) {
			dir.remove(file);
		}
	}
}

//-----------------------------------------------------------------------------

QString Theme::createId()
{
	QString file;
	do
	{
		file = QUuid::createUuid().toString().mid(1, 36);
	} while (QFile::exists(filePath(file, false)));
	return file;
}

//-----------------------------------------------------------------------------

bool Theme::exists(const QString& name)
{
	QDir dir(m_path, "*.theme");
	QStringList themes = dir.entryList(QDir::Files);
	for (const QString& theme : themes) {
		QSettings settings(dir.filePath(theme), QSettings::IniFormat);
		if (settings.value("Name").toString() == name) {
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

QString Theme::filePath(const QString& id, bool is_default)
{
	return (!is_default ? m_path : m_path_default) + "/" + id + ".theme";
}

//-----------------------------------------------------------------------------

QString Theme::iconPath(const QString& id, bool is_default, qreal pixelratio)
{
	QString pixel;
	if (pixelratio > 1.0) {
		pixel = QString("@%1x").arg(pixelratio);
	}
	return m_path + (!is_default ? "/Previews/" : "/Previews/Default/") + id + pixel + ".png";
}

//-----------------------------------------------------------------------------

void Theme::removeIcon(const QString& id, bool is_default)
{
	QDir dir = listIcons(id, is_default);
	const QStringList files = dir.entryList();
	for (const QString& file : files) {
		dir.remove(file);
	}
}

//-----------------------------------------------------------------------------

void Theme::setDefaultPath(const QString& path)
{
	m_path_default = path;
}

//-----------------------------------------------------------------------------

void Theme::setPath(const QString& path)
{
	m_path = path;
	if (m_path_default.isEmpty()) {
		m_path_default = m_path;
	}
}

//-----------------------------------------------------------------------------

QImage Theme::render(const QSize& background, QRect& foreground, const int margin, const qreal pixelratio) const
{
	// Create image
	QImage image(background * pixelratio, QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(pixelratio);
	image.fill(backgroundColor().rgb());

	QPainter painter(&image);
	painter.setPen(Qt::NoPen);

	// Draw background image
	if (backgroundType() > 1) {
		QImageReader source(backgroundImage());
		QSize scaled = source.size();
		switch (backgroundType()) {
		case 3:
			// Stretched
			scaled.scale(background, Qt::IgnoreAspectRatio);
			break;
		case 4:
			// Scaled
			scaled.scale(background, Qt::KeepAspectRatio);
			break;
		case 5:
			// Zoomed
			scaled.scale(background, Qt::KeepAspectRatioByExpanding);
			break;
		default:
			// Centered
			scaled /= pixelratio;
			break;
		}
		source.setScaledSize(scaled * pixelratio);

		QImage back = source.read();
		back.setDevicePixelRatio(pixelratio);

		painter.drawImage(QPointF((background.width() - scaled.width()) / 2, (background.height() - scaled.height()) / 2), back);
	} else if (backgroundType() == 1) {
		// Tiled
		QImage back(backgroundImage());
		back.setDevicePixelRatio(pixelratio);
		painter.save();
		painter.scale(1.0 / pixelratio, 1.0 / pixelratio);
		painter.fillRect(QRectF(image.rect()), back);
		painter.restore();
	}

	// Determine foreground rectangle
	foreground = foregroundRect(background, margin, pixelratio);

	// Set clipping for rounded themes
	QPainterPath path;
	if (roundCornersEnabled()) {
		painter.setRenderHint(QPainter::Antialiasing);
		path.addRoundedRect(QRectF(foreground), cornerRadius(), cornerRadius());
		painter.setClipPath(path);
	} else {
		path.addRect(foreground);
	}

	// Blur behind foreground
	if (blurEnabled()) {
		QImage blurred = image.copy(QRect(foreground.topLeft() * pixelratio, foreground.bottomRight() * pixelratio));

		painter.save();
		painter.translate(foreground.x(), foreground.y());
		qt_blurImage(&painter, blurred, blurRadius() * 2, true, false);
		painter.restore();
	}

	// Draw drop shadow
	int shadow_radius = shadowEnabled() ? shadowRadius() : 0;
	if (shadow_radius) {
		QImage copy = image.copy(QRect(foreground.topLeft() * pixelratio, foreground.bottomRight() * pixelratio));

		QImage shadow(background, QImage::Format_ARGB32_Premultiplied);
		shadow.fill(0);

		QPainter shadow_painter(&shadow);
		shadow_painter.setRenderHint(QPainter::Antialiasing);
		shadow_painter.setPen(Qt::NoPen);
		shadow_painter.translate(0, shadowOffset());
		shadow_painter.fillPath(path, shadowColor());
		shadow_painter.end();

		painter.save();
		painter.setClipping(false);
		qt_blurImage(&painter, shadow, shadow_radius * 2, true, false);
		painter.setClipping(roundCornersEnabled());
		painter.restore();

		painter.drawImage(QPointF(foreground.x(), foreground.y()), copy);
	}

	// Draw foreground
	QColor color = foregroundColor();
	color.setAlpha(foregroundOpacity() * 2.55f);
	painter.fillRect(QRectF(foreground), color);

	return image;
}

//-----------------------------------------------------------------------------

void Theme::renderText(QImage background, const QRect& foreground, const qreal pixelratio, QImage* preview, QImage* icon) const
{
	// Create preview text
	QTextEdit preview_text;
	preview_text.setFrameStyle(QFrame::NoFrame);
	preview_text.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	preview_text.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QFile file(":/lorem.txt");
	if (file.open(QFile::ReadOnly)) {
		preview_text.setPlainText(QString::fromLatin1(file.readAll()));
		file.close();
	}

	// Position preview text
	int padding = foregroundPadding();
	int x = foreground.x() + padding;
	int y = foreground.y() + padding + spacingAboveParagraph();
	int width = foreground.width() - (padding * 2);
	int height = foreground.height() - (padding * 2) - spacingAboveParagraph();
	preview_text.setGeometry(x, y, width, height);

	// Set colors
	QColor text_color = textColor();
	text_color.setAlpha(255);

	QPalette p = preview_text.palette();
	p.setBrush(QPalette::Window, Qt::transparent);
	p.setBrush(QPalette::Base, Qt::transparent);
	p.setColor(QPalette::Text, text_color);
	p.setColor(QPalette::Highlight, text_color);
	p.setColor(QPalette::HighlightedText, (qGray(text_color.rgb()) > 127) ? Qt::black : Qt::white);
	preview_text.setPalette(p);

	// Set spacings
	int tab_width = tabWidth();
	QTextBlockFormat block_format;
	block_format.setLineHeight(lineSpacing(), (lineSpacing() == 100) ? QTextBlockFormat::SingleHeight : QTextBlockFormat::ProportionalHeight);
	block_format.setTextIndent(tab_width * indentFirstLine());
	block_format.setTopMargin(spacingAboveParagraph());
	block_format.setBottomMargin(spacingBelowParagraph());
	preview_text.textCursor().mergeBlockFormat(block_format);
	for (int i = 0, count = preview_text.document()->allFormats().count(); i < count; ++i) {
		QTextFormat& f = preview_text.document()->allFormats()[i];
		if (f.isBlockFormat()) {
			f.merge(block_format);
		}
	}
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
	preview_text.setTabStopDistance(tab_width);
#else
	preview_text.setTabStopWidth(tab_width);
#endif
	preview_text.document()->setIndentWidth(tab_width);

	// Set font
	preview_text.setFont(textFont());

	// Render text
	preview_text.render(&background, preview_text.pos());

	// Create preview pixmap
	if (preview) {
		*preview = background.scaled(480 * pixelratio, 270 * pixelratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		QPainter painter(preview);
		painter.setPen(Qt::NoPen);

		// Draw text cutout shadow
		painter.fillRect(QRectF(22, 46, 170, 118), QColor(0, 0, 0, 32));
		painter.fillRect(QRectF(24, 48, 166, 114), Qt::white);

		// Draw text cutout
		int x2 = (x >= 24) ? (x - 24) : 0;
		int y2 = (y >= 24) ? (y - 24) : 0;
		painter.drawImage(QPointF(26, 50), background, QRectF(x2 * pixelratio, y2 * pixelratio, 162 * pixelratio, 110 * pixelratio));
	}

	// Create preview icon
	if (icon) {
		*icon = QImage(258 * pixelratio, 153 * pixelratio, QImage::Format_ARGB32_Premultiplied);
		icon->fill(Qt::transparent);

		// Draw shadow
		QImage shadow(*icon);

		QPainter painter(&shadow);
		painter.setPen(Qt::NoPen);
		painter.scale(pixelratio, pixelratio);
		painter.fillRect(QRectF(9, 10, 240, 135), Qt::black);
		painter.end();

		painter.begin(icon);
		qt_blurImage(&painter, shadow, 10 * pixelratio, true, false);
		painter.end();

		// Draw preview
		icon->setDevicePixelRatio(pixelratio);
		painter.begin(icon);
		painter.drawImage(QPointF(9, 9), background.scaled(240 * pixelratio, 135 * pixelratio, Qt::KeepAspectRatio, Qt::SmoothTransformation));

		// Draw text cutout shadow
		painter.fillRect(QRectF(20, 32, 85, 59), QColor(0, 0, 0, 32));
		painter.fillRect(QRectF(21, 33, 83, 57), Qt::white);

		// Draw text cutout
		int x2 = (x >= 24) ? (x - 12 + tabWidth()) : 12 + tabWidth();
		int y2 = (y >= 24) ? (y - 6) : 0;
		painter.drawImage(QPointF(22, 34), background, QRectF(x2 * pixelratio, y2 * pixelratio, 81 * pixelratio, 55 * pixelratio));
	}
}

//-----------------------------------------------------------------------------

QFuture<QColor> Theme::calculateLoadColor() const
{
	return QtConcurrent::run(averageImage, backgroundImage(), backgroundColor());
}

//-----------------------------------------------------------------------------

QString Theme::backgroundImage() const
{
	if (!d->is_default) {
		return m_path + "/Images/" + d->background_image;
	} else {
		return m_path_default + "/images/" + d->background_image;
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

QRect Theme::foregroundRect(const QSize& size, int margin, const qreal pixelratio) const
{
	margin = std::max(margin, d->foreground_margin.value());
	int x = 0;
	int y = margin;
	int width = std::min(d->foreground_width.value(), size.width() - (margin * 2));
	int height = size.height() - (margin * 2);

	switch (d->foreground_position) {
	case 0:
		// Left
		x = margin;
		break;
	case 2:
		// Right
		x = size.width() - margin - width;
		break;
	case 3:
		// Stretched
		x = margin;
		width = size.width() - (margin * 2);
		break;
	case 1:
	default:
		// Centered
		x = (size.width() - width) / 2;
		break;
	};

	width = std::floor(std::floor(width / pixelratio) * pixelratio);
	height = std::floor(std::floor(height / pixelratio) * pixelratio);

	return QRect(x, y, width, height);
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
		&& (d->foreground_margin == theme.d->foreground_margin)
		&& (d->foreground_padding == theme.d->foreground_padding)
		&& (d->foreground_position == theme.d->foreground_position)

		&& (d->round_corners_enabled == theme.d->round_corners_enabled)
		&& (d->corner_radius == theme.d->corner_radius)

		&& (d->blur_enabled == theme.d->blur_enabled)
		&& (d->blur_radius == theme.d->blur_radius)

		&& (d->shadow_enabled == theme.d->shadow_enabled)
		&& (d->shadow_offset == theme.d->shadow_offset)
		&& (d->shadow_radius == theme.d->shadow_radius)
		&& (d->shadow_color == theme.d->shadow_color)

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
	if (d->id.isEmpty()) {
		return;
	}

	QSettings settings(filePath(d->id, d->is_default), QSettings::IniFormat);

	d->name = settings.value("Name", d->name).toString();

	// Load background settings
	d->background_type = settings.value("Background/Type", 0).toInt();
	d->background_color = settings.value("Background/Color", "#666666").toString();
	d->background_path = settings.value("Background/Image").toString();
	d->background_image = settings.value("Background/ImageFile").toString();
	if (!d->background_path.isEmpty() && d->background_image.isEmpty()) {
		setValue(d->background_image, copyImage(d->background_path));
	}

	d->load_color = settings.value("LoadColor", d->background_color.name()).toString();

	// Load foreground settings
	d->foreground_color = settings.value("Foreground/Color", "#ffffff").toString();
	d->foreground_opacity = settings.value("Foreground/Opacity", 100).toInt();
	d->foreground_width = settings.value("Foreground/Width", 700).toInt();
	d->foreground_margin = settings.value("Foreground/Margin", 65).toInt();
	d->foreground_padding = settings.value("Foreground/Padding", 10).toInt();
	d->foreground_position = settings.value("Foreground/Position", 1).toInt();

	int rounding = settings.value("Foreground/Rounding", 0).toInt();
	if (rounding > 0) {
		d->round_corners_enabled = true;
		d->corner_radius = rounding;
	} else {
		d->round_corners_enabled = false;
		d->corner_radius = settings.value("Foreground/RoundingDisabled", 10).toInt();
	}

	d->blur_enabled = settings.value("ForegroundBlur/Enabled", false).toBool();
	d->blur_radius = settings.value("ForegroundBlur/Radius", 32).toInt();

	d->shadow_enabled = settings.value("ForegroundShadow/Enabled", !settings.contains("Foreground/Color")).toBool();
	d->shadow_color = settings.value("ForegroundShadow/Color", "#000000").toString();
	d->shadow_radius = settings.value("ForegroundShadow/Radius", 8).toInt();
	d->shadow_offset = settings.value("ForegroundShadow/Offset", 2).toInt();

	// Load text settings
	d->text_color = settings.value("Text/Color", "#000000").toString();
	d->text_font.fromString(settings.value("Text/Font", QFont("Times New Roman").toString()).toString());
	if (d->is_default) {
#if defined(Q_OS_MAC)
		int point_size = 14;
#elif defined(Q_OS_UNIX)
		int point_size = 10;
#else
		int point_size = 12;
#endif
		d->text_font.setPointSize(std::max(point_size, QFont().pointSize()));
	}
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

	if (d->is_default) {
		if (!d->background_image.isEmpty()) {
			d->background_image = copyImage(m_path_default + "/images/" + d->background_image);
		}
	}
	d->is_default = false;

	QSettings settings(filePath(d->id), QSettings::IniFormat);

	settings.setValue("LoadColor", d->load_color.name());
	settings.setValue("Name", d->name);

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
	settings.setValue("Foreground/Margin", d->foreground_margin.value());
	settings.setValue("Foreground/Padding", d->foreground_padding.value());
	settings.setValue("Foreground/Position", d->foreground_position.value());

	if (d->round_corners_enabled) {
		settings.setValue("Foreground/Rounding", d->corner_radius.value());
		settings.setValue("Foreground/RoundingDisabled", 0);
	} else {
		settings.setValue("Foreground/Rounding", 0);
		settings.setValue("Foreground/RoundingDisabled", d->corner_radius.value());
	}

	settings.setValue("ForegroundBlur/Enabled", d->blur_enabled);
	settings.setValue("ForegroundBlur/Radius", d->blur_radius.value());

	settings.setValue("ForegroundShadow/Enabled", d->shadow_enabled);
	settings.setValue("ForegroundShadow/Color", d->shadow_color.name());
	settings.setValue("ForegroundShadow/Radius", d->shadow_radius.value());
	settings.setValue("ForegroundShadow/Offset", d->shadow_offset.value());

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
