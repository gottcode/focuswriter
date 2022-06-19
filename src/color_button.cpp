/*
	SPDX-FileCopyrightText: 2008-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "color_button.h"

#include <QColorDialog>
#include <QPainter>
#include <QPixmap>

//-----------------------------------------------------------------------------

ColorButton::ColorButton(QWidget* parent)
	: QPushButton(parent)
{
	setAutoDefault(false);
	connect(this, &ColorButton::clicked, this, &ColorButton::onClicked);
}

//-----------------------------------------------------------------------------

QString ColorButton::toString() const
{
	return m_color.name();
}

//-----------------------------------------------------------------------------

void ColorButton::setColor(const QColor& color)
{
	if (m_color == color) {
		return;
	}
	m_color = color;

	QPixmap swatch(75, fontMetrics().height());
	swatch.fill(m_color);
	{
		QPainter painter(&swatch);
		painter.setPen(m_color.darker());
		painter.drawRect(QRectF(0, 0, swatch.width() - 1, swatch.height() - 1));
		painter.setPen(m_color.lighter());
		painter.drawRect(QRectF(1, 1, swatch.width() - 3, swatch.height() - 3));
	}
	setIconSize(swatch.size());
	setIcon(swatch);

	Q_EMIT changed(m_color);
}

//-----------------------------------------------------------------------------

void ColorButton::onClicked()
{
	const QColor color = QColorDialog::getColor(m_color, this);
	if (color.isValid()) {
		setColor(color);
	}
}

//-----------------------------------------------------------------------------
