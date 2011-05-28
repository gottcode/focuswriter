/***********************************************************************
 *
 * Copyright (C) 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "alert.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimeLine>
#include <QToolButton>

//-----------------------------------------------------------------------------

Alert::Alert(const QPixmap& pixmap, const QString& text, const QStringList& details, QWidget* parent)
	: QWidget(parent),
	m_expanded(true),
	m_under_mouse(false)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setStyleSheet("QLabel { color: white } Alert { color: white; background-color: black }");

	if (parent) {
		parent->installEventFilter(this);
	}

	m_expander = new QToolButton(this);
	m_expander->setAutoRaise(true);
	m_expander->setIconSize(QSize(16,16));
	m_expander->hide();
	connect(m_expander, SIGNAL(clicked()), this, SLOT(expanderToggled()));

	m_pixmap = new QLabel(this);
	m_pixmap->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

	m_text = new QLabel(this);
	m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
	m_text->setWordWrap(true);

	QToolButton* close = new QToolButton(this);
	close->setAutoRaise(true);
	close->setIconSize(QSize(16,16));
	close->setIcon(QIcon::fromTheme("window-close"));
	close->setToolTip(tr("Close (Ctrl+D)"));
	connect(close, SIGNAL(clicked()), this, SLOT(fadeOut()));

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(7);
	layout->setSpacing(6);
	layout->addWidget(m_expander, 0, Qt::AlignHCenter | Qt::AlignBottom);
	layout->addWidget(m_pixmap);
	layout->addWidget(m_text, 1);
	layout->addWidget(close, 0, Qt::AlignHCenter | Qt::AlignTop);

	setText(pixmap, text, details);
	expanderToggled();

	QGraphicsOpacityEffect* fade_effect = new QGraphicsOpacityEffect(this);
	fade_effect->setOpacity(0.0);
	setGraphicsEffect(fade_effect);

	m_fade_timer = new QTimeLine(240, this);
	connect(m_fade_timer, SIGNAL(valueChanged(qreal)), fade_effect, SLOT(setOpacity(qreal)));
}

//-----------------------------------------------------------------------------

void Alert::fadeIn()
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	connect(m_fade_timer, SIGNAL(finished()), this, SLOT(fadeInFinished()));
	m_fade_timer->start();
}

//-----------------------------------------------------------------------------

void Alert::setText(const QPixmap& pixmap, const QString& text, const QStringList& details)
{
	if (!pixmap.isNull()) {
		m_pixmap->setPixmap(pixmap);
	} else {
		m_pixmap->clear();
	}

	m_short_text = "<p>" + text + "</p>";
	m_long_text = "<p>" + text + "<br><small>" + details.join("<br>") + "</small></p>";

	m_text->setText(m_short_text);
	m_expander->setVisible(!details.isEmpty());
}

//-----------------------------------------------------------------------------

bool Alert::eventFilter(QObject* watched, QEvent* event)
{
	if ((watched == parentWidget()) && (event->type() == QEvent::Resize)) {
		m_under_mouse = rect().contains(mapFromGlobal(QCursor::pos()));
		update();
	}
	return QWidget::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------

void Alert::enterEvent(QEvent* event)
{
	m_under_mouse = true;
	update();
	QWidget::enterEvent(event);
}

//-----------------------------------------------------------------------------

void Alert::leaveEvent(QEvent* event)
{
	m_under_mouse = false;
	update();
	QWidget::leaveEvent(event);
}

//-----------------------------------------------------------------------------

void Alert::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setOpacity(m_under_mouse ? 1.0 : 0.8);
	painter.setPen(QPen(palette().color(foregroundRole()), 1.5));
	painter.setBrush(palette().color(backgroundRole()));
	painter.drawRoundedRect(QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5), 7, 7);
}

//-----------------------------------------------------------------------------

void Alert::expanderToggled()
{
	m_expanded = !m_expanded;
	if (m_expanded) {
		m_expander->setIcon(QIcon::fromTheme("arrow-up"));
		m_expander->setToolTip(tr("Collapse"));
		m_text->setText(m_long_text);
	} else {
		m_expander->setIcon(QIcon::fromTheme("arrow-right"));
		m_expander->setToolTip(tr("Expand"));
		m_text->setText(m_short_text);
	}
}

//-----------------------------------------------------------------------------

void Alert::fadeInFinished()
{
	setAttribute(Qt::WA_TransparentForMouseEvents, false);
	disconnect(m_fade_timer, SIGNAL(finished()), this, SLOT(fadeInFinished()));
}

//-----------------------------------------------------------------------------

void Alert::fadeOut()
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	m_fade_timer->setDirection(QTimeLine::Backward);
	connect(m_fade_timer, SIGNAL(finished()), this, SLOT(deleteLater()));
	m_fade_timer->start();
}

//-----------------------------------------------------------------------------
