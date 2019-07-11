/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2012, 2014, 2016, 2018, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "image_button.h"

#include <QFileDialog>
#include <QImageReader>
#include <QPainter>
#include <QSettings>
#include <QStandardPaths>

//-----------------------------------------------------------------------------

ImageButton::ImageButton(QWidget* parent)
	: QPushButton(parent)
{
	setAutoDefault(false);
	setIconSize(QSize(100, 100));
	unsetImage();
	connect(this, &ImageButton::clicked, this, &ImageButton::onClicked);
}

//-----------------------------------------------------------------------------

void ImageButton::setImage(const QString& image, const QString& path)
{
	QImageReader source(image);
	if (source.canRead()) {
		m_image = image;

		// Find scaled size
		const qreal pixelratio = devicePixelRatioF();
		const int edge = 100 * pixelratio;
		QSize size = source.size();
		if (size.width() > edge || size.height() > edge) {
			size.scale(QSize(edge, edge), Qt::KeepAspectRatio);
			source.setScaledSize(size);
		}

		// Create square icon with image centered
		QImage icon(QSize(edge, edge), QImage::Format_ARGB32_Premultiplied);
		icon.fill(Qt::transparent);
		QPainter painter(&icon);
		painter.drawImage((edge - size.width()) / 2, (edge - size.height()) / 2, source.read());
		painter.end();

		// Load icon
		icon.setDevicePixelRatio(pixelratio);
		setIcon(QPixmap::fromImage(icon, Qt::AutoColor | Qt::AvoidDither));

		m_path = (!path.isEmpty() && QImageReader(path).canRead()) ? path : QString();
		emit changed(m_path);
	} else {
		unsetImage();
	}
}

//-----------------------------------------------------------------------------

void ImageButton::unsetImage()
{
	m_image.clear();
	m_path.clear();

	const qreal pixelratio = devicePixelRatioF();
	QPixmap icon(QSize(100, 100) * pixelratio);
	icon.setDevicePixelRatio(pixelratio);
	icon.fill(Qt::transparent);
	setIcon(icon);

	emit changed(m_path);
}

//-----------------------------------------------------------------------------

void ImageButton::onClicked()
{
	QStringList filters;
	QList<QByteArray> formats = QImageReader::supportedImageFormats();
	for (const QByteArray& type : formats) {
		filters.append("*." + type);
	}

	QSettings settings;
	QString path = m_path;
	if (path.isEmpty() || !QFile::exists(path)) {
		path = settings.value("ImageButton/Location").toString();
		if (path.isEmpty() || !QFile::exists(path)) {
			path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
		}
	}

	QString image = QFileDialog::getOpenFileName(window(), tr("Open Image"), path, tr("Images(%1)").arg(filters.join(" ")));
	if (!image.isEmpty()) {
		settings.setValue("ImageButton/Location", QFileInfo(image).absolutePath());
		setImage(image, image);
	}
}

//-----------------------------------------------------------------------------
