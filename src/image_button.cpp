/*
	SPDX-FileCopyrightText: 2008-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

		m_path = path;
		Q_EMIT changed(m_path);
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

	Q_EMIT changed(m_path);
}

//-----------------------------------------------------------------------------

void ImageButton::onClicked()
{
	QStringList filters;
	const QList<QByteArray> formats = QImageReader::supportedImageFormats();
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

	const QString image = QFileDialog::getOpenFileName(window(), tr("Open Image"), path, tr("Images") + QString(" (%1)").arg(filters.join(" ")));
	if (!image.isEmpty()) {
		settings.setValue("ImageButton/Location", QFileInfo(image).absolutePath());
		setImage(image, image);
	}
}

//-----------------------------------------------------------------------------
