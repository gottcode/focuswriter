/***********************************************************************
 *
 * Copyright (C) 2010, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "timer_display.h"

#include "timer.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTime>
#include <QTimer>
#include <QToolTip>

//-----------------------------------------------------------------------------

TimerDisplay::TimerDisplay(QList<Timer*>& timers, QWidget* parent)
	: QWidget(parent),
	m_show_tip(false),
	m_timer(0),
	m_timers(timers)
{
	m_size = fontMetrics().height();

	m_update_timer = new QTimer(this);
	m_update_timer->setInterval(40);
	m_update_timer->start();
	connect(m_update_timer, &QTimer::timeout, this, QOverload<>::of(&TimerDisplay::update));
}

//-----------------------------------------------------------------------------

void TimerDisplay::setTimer(Timer* timer)
{
	m_timer = timer;
	if (m_timer) {
		m_update_timer->start();
	} else {
		m_update_timer->stop();
	}
	update();
}

//-----------------------------------------------------------------------------

QSize TimerDisplay::minimumSizeHint() const
{
	return QSize(m_size, m_size);
}

//-----------------------------------------------------------------------------

QSize TimerDisplay::sizeHint() const
{
	return QSize(m_size, m_size);
}

//-----------------------------------------------------------------------------

void TimerDisplay::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::FontChange) {
		m_size = fontMetrics().height();
		updateGeometry();
	}
	QWidget::changeEvent(event);
}

//-----------------------------------------------------------------------------

bool TimerDisplay::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip) {
		m_show_tip = true;
		m_tip_pos = static_cast<QHelpEvent*>(event)->globalPos();
		update();
	}
	return QWidget::event(event);
}

//-----------------------------------------------------------------------------

void TimerDisplay::hideEvent(QHideEvent* event)
{
	m_update_timer->stop();
	QWidget::hideEvent(event);
}

//-----------------------------------------------------------------------------

void TimerDisplay::leaveEvent(QEvent* event)
{
	m_show_tip = false;
	QToolTip::hideText();
	QWidget::leaveEvent(event);
}

//-----------------------------------------------------------------------------

void TimerDisplay::mouseReleaseEvent(QMouseEvent* event)
{
	if ((event->button() == Qt::LeftButton) && rect().contains(event->pos())) {
		emit clicked();
	}
	QWidget::mouseReleaseEvent(event);
}

//-----------------------------------------------------------------------------

void TimerDisplay::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QRectF rect(1.5, 1.5, m_size - 3, m_size - 3);
	if (m_timer) {
		QDateTime current = QDateTime::currentDateTime();
		int degrees = (m_timer->msecsFrom(current) * -5760.0) / m_timer->msecsTotal();

		painter.setPen(palette().color(QPalette::WindowText));
		painter.drawEllipse(rect);

		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::black);
		painter.drawPie(rect, 1440, degrees);

		if (m_show_tip) {
			QStringList timers;
			for (Timer* timer : m_timers) {
				if (timer->isRunning()) {
					int msecs = timer->msecsFrom(current);
					timers += Timer::toString(QTime().addMSecs(msecs).toString(tr("HH:mm:ss")), timer->memoShort());
				}
			}
			QString text = QLatin1String("<p style='white-space:pre'>") + timers.join(QLatin1String("\n")) + QLatin1String("</p>");
			QToolTip::showText(m_tip_pos, text, this, this->rect());
		}
	} else {
		painter.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));
		painter.drawEllipse(rect);
		if (m_show_tip) {
			QToolTip::showText(m_tip_pos, tr("No timers running"), this, this->rect());
		}
	}
}

//-----------------------------------------------------------------------------

void TimerDisplay::showEvent(QShowEvent* event)
{
	if (m_timer) {
		m_update_timer->start();
	}
	QWidget::showEvent(event);
}

//-----------------------------------------------------------------------------
