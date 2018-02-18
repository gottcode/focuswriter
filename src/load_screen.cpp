/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2016, 2018 Graeme Gott <graeme@gottcode.org>
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

#include "load_screen.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

LoadScreen::LoadScreen(QWidget* parent) :
	QLabel(parent)
{
	setCursor(Qt::WaitCursor);

	QPalette p = palette();
	p.setColor(backgroundRole(), "#666");
	setPalette(p);
	setAutoFillBackground(true);

	QString px;
	qreal pixelratio = devicePixelRatioF();
	if (pixelratio > 2.0) {
		pixelratio = 3.0;
		px = "@3x";
	} else if (pixelratio > 1.0) {
		pixelratio = 2.0;
		px = "@2x";
	} else {
		pixelratio = 1.0;
	}
	m_pixmap.load(QString(":/load%1.png").arg(px));
	m_pixmap.setDevicePixelRatio(pixelratio);
	m_pixmap_center = m_pixmap.size() / (2 * pixelratio);

	m_text = new QLabel(this);
	m_text->hide();
	m_text->setCursor(Qt::WaitCursor);
	m_text->setAlignment(Qt::AlignCenter);
	m_text->setStyleSheet("QLabel {color: #d7d7d7; background-color: #1e1e1e; border-top-left-radius: 0.25em; border-top-right-radius: 0.25em; padding: 0.25em 0.5em;}");

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->addStretch();
	layout->addWidget(m_text, 0, Qt::AlignCenter);

	m_hide_effect = new QGraphicsOpacityEffect(this);
	m_hide_effect->setOpacity(1.0);
	setGraphicsEffect(m_hide_effect);
	m_hide_effect->setEnabled(false);

	m_hide_timer = new QTimer(this);
	m_hide_timer->setInterval(30);
	connect(m_hide_timer, SIGNAL(timeout()), this, SLOT(fade()));
}

//-----------------------------------------------------------------------------

bool LoadScreen::eventFilter(QObject* watched, QEvent* event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::MouseButtonDblClick:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
		if (!window()->isActiveWindow()) {
			return QLabel::eventFilter(watched, event);
		}
		// fall through

	case QEvent::Shortcut:
	case QEvent::Wheel:
		return true;

	default:
		break;
	}
	return QLabel::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------

void LoadScreen::setText(const QString& step)
{
	m_text->setText("<pre>" + step + "</pre>");
	m_text->setVisible(!step.isEmpty());

	if (m_hide_timer->isActive()) {
		m_hide_timer->stop();
	}
	m_hide_effect->setOpacity(1.0);
	m_hide_effect->setEnabled(false);

	show();
	raise();
	QApplication::processEvents();
}

//-----------------------------------------------------------------------------

void LoadScreen::finish()
{
	m_hide_effect->setOpacity(1.0);
	m_hide_effect->setEnabled(true);
	m_hide_timer->start();
}

//-----------------------------------------------------------------------------

void LoadScreen::hideEvent(QHideEvent* event)
{
	QLabel::hideEvent(event);
	QApplication::instance()->removeEventFilter(this);
}

//-----------------------------------------------------------------------------

void LoadScreen::showEvent(QShowEvent* event)
{
	QLabel::showEvent(event);
	QApplication::instance()->installEventFilter(this);
}

//-----------------------------------------------------------------------------

void LoadScreen::paintEvent(QPaintEvent* event)
{
	QLabel::paintEvent(event);
	QPainter painter(this);
	const QSizeF sz = size() / 2;
	painter.drawPixmap(sz.width() - m_pixmap_center.width(), sz.height() - m_pixmap_center.height(), m_pixmap);
}

//-----------------------------------------------------------------------------

void LoadScreen::fade()
{
	m_hide_effect->setOpacity(m_hide_effect->opacity() - 0.2);
	if (m_hide_effect->opacity() <= 0.01) {
		m_hide_timer->stop();
		hide();
		m_hide_effect->setEnabled(false);
	}
}

//-----------------------------------------------------------------------------
