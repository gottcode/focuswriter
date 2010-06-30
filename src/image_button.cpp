/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

/*****************************************************************************/

ImageButton::ImageButton(QWidget* parent)
: QPushButton(parent) {
	setAutoDefault(false);
	setIconSize(QSize(100, 100));
	unsetImage();
	connect(this, SIGNAL(clicked()), this, SLOT(onClicked()));
}

/*****************************************************************************/

void ImageButton::setImage(const QString& path) {
	m_path = path;
	m_image.load(path);

	if (m_image.width() > 100 || m_image.height() > 100) {
		QImage icon = m_image.scaled(QSize(100, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		setIcon(QPixmap::fromImage(icon));
	}

	emit changed(m_image);
}

/*****************************************************************************/

void ImageButton::unsetImage() {
	m_path.clear();
	m_image = QImage();

	QPixmap icon(100,100);
	icon.fill(Qt::transparent);
	setIcon(icon);

	emit changed(m_image);
}

/*****************************************************************************/

void ImageButton::onClicked() {
	QStringList filters;
	QList<QByteArray> formats = QImageReader::supportedImageFormats();
	foreach (QByteArray type, formats) {
		filters.append("*." + type);
	}
	QString image = QFileDialog::getOpenFileName(window(), tr("Open Image"), m_path, tr("Images (%1)").arg(filters.join(" ")));
	if (!image.isEmpty()) {
		setImage(image);
	}
}

/*****************************************************************************/
