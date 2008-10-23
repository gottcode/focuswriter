/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "image_dialog.h"

/*****************************************************************************/

ImageButton::ImageButton(const QString& path, QWidget* parent)
: QPushButton(parent) {
	setAutoDefault(false);
	setIconSize(QSize(100, 100));
	setMinimumSize(QSize(100, 100));
	setImage(path);
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

void ImageButton::onClicked() {
	ImageDialog dialog(window());
	dialog.setDirectory(m_path);
	if (dialog.exec()) {
		setImage(dialog.selectedFiles().first());
	}
}

/*****************************************************************************/
