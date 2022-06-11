/*
	SPDX-FileCopyrightText: 2010-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "timer_display.h"

#include "timer.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTime>
#include <QTimer>
#include <QToolTip>

//-----------------------------------------------------------------------------

TimerDisplay::TimerDisplay(const QList<Timer*>& timers, QWidget* parent)
	: QWidget(parent)
	, m_show_tip(false)
	, m_timer(nullptr)
	, m_timers(timers)
{
	m_size = fontMetrics().height();

	m_update_timer = new QTimer(this);
	m_update_timer->setInterval(40);
	m_update_timer->start();
	connect(m_update_timer, &QTimer::timeout, this, qOverload<>(&TimerDisplay::update));
}

//-----------------------------------------------------------------------------

void TimerDisplay::setTimer(const Timer* timer)
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
		Q_EMIT clicked();
	}
	QWidget::mouseReleaseEvent(event);
}

//-----------------------------------------------------------------------------

void TimerDisplay::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	const QRectF rect(1.5, 1.5, m_size - 3, m_size - 3);
	if (m_timer) {
		const QDateTime current = QDateTime::currentDateTime();
		const int degrees = (m_timer->msecsFrom(current) * -5760.0) / m_timer->msecsTotal();

		painter.setPen(palette().color(QPalette::WindowText));
		painter.drawEllipse(rect);

		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::black);
		painter.drawPie(rect, 1440, degrees);

		if (m_show_tip) {
			QStringList timers;
			for (const Timer* timer : m_timers) {
				if (timer->isRunning()) {
					const int msecs = timer->msecsFrom(current);
					timers += Timer::toString(QTime(0, 0, 0).addMSecs(msecs).toString(tr("HH:mm:ss")), timer->memoShort());
				}
			}
			const QString text = QLatin1String("<p style='white-space:pre'>") + timers.join(QLatin1String("\n")) + QLatin1String("</p>");
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
