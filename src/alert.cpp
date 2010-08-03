/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QTimeLine>
#include <QToolButton>

//-----------------------------------------------------------------------------

Alert::Alert(const QString& text, const QStringList& details, QWidget* parent)
	: QWidget(parent),
	m_expanded(true),
	m_under_mouse(false)
{
	setAttribute(Qt::WA_NoSystemBackground);
	setStyleSheet("QLabel { color: white } Alert { color: white; background-color: black }");

	if (parent) {
		parent->installEventFilter(this);
	}

	m_expander = new QToolButton(this);
	m_expander->setAutoRaise(true);
	m_expander->setIconSize(QSize(16,16));
	m_expander->hide();
	connect(m_expander, SIGNAL(clicked()), this, SLOT(expanderToggled()));

	m_text = new QLabel(this);
	m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
	m_text->setWordWrap(true);

	QToolButton* close = new QToolButton(this);
	close->setAutoRaise(true);
	close->setIconSize(QSize(16,16));
	close->setIcon(QIcon::fromTheme("window-close"));
	close->setToolTip(tr("Ctrl+D"));
	connect(close, SIGNAL(clicked()), this, SLOT(fadeOut()));

	m_details_layout = new QVBoxLayout;

	QGridLayout* layout = new QGridLayout(this);
	layout->setMargin(7);
	layout->setSpacing(0);
	layout->setColumnMinimumWidth(1, 6);
	layout->setColumnMinimumWidth(3, 6);
	layout->setColumnStretch(2, 1);
	layout->addWidget(m_expander, 0, 0, 2, 1, Qt::AlignHCenter | Qt::AlignBottom);
	layout->addWidget(m_text, 0, 2);
	layout->addWidget(close, 0, 4, Qt::AlignHCenter | Qt::AlignTop);
	layout->addLayout(m_details_layout, 1, 2);

	setText(text, details);
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

void Alert::setText(const QString& text, const QStringList& details)
{
	qDeleteAll(m_details);
	m_details.clear();

	m_text->setText(text);

	foreach (const QString& detail, details) {
		QLabel* label = new QLabel("<small>" + detail + "</small>", this);
		label->setTextInteractionFlags(Qt::TextSelectableByMouse);
		label->setWordWrap(true);
		m_details.append(label);
		m_details_layout->addWidget(label);
	}
	m_expander->setVisible(!m_details.isEmpty());
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
	m_expander->setIcon(QIcon::fromTheme(m_expanded ? "arrow-up" : "arrow-right"));
	foreach (QLabel* detail, m_details) {
		detail->setVisible(m_expanded);
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
