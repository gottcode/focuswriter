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

#include "alert_layer.h"

#include "alert.h"

#include <QShortcut>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

AlertLayer::AlertLayer(QWidget* parent)
	: QWidget(parent)
{
	setMaximumWidth(400);
	m_alerts_layout = new QVBoxLayout(this);
	m_alerts_layout->setMargin(0);
	new QShortcut(tr("Ctrl+D"), this, SLOT(dismissAlert()));
}

//-----------------------------------------------------------------------------

void AlertLayer::addAlert(const QPixmap& pixmap, const QString& text, const QStringList& details)
{
	Alert* alert = new Alert(pixmap, text, details, this);
	m_alerts.append(alert);
	m_alerts_layout->addWidget(alert);
	connect(alert, SIGNAL(destroyed(QObject*)), this, SLOT(alertDestroyed(QObject*)));
	alert->fadeIn();
}

//-----------------------------------------------------------------------------

void AlertLayer::alertDestroyed(QObject* alert)
{
	for (int i = 0; i < m_alerts.count(); ++i) {
		if (m_alerts[i] == alert) {
			m_alerts.removeAt(i);
			return;
		}
	}
}

//-----------------------------------------------------------------------------

void AlertLayer::dismissAlert()
{
	if (!m_alerts.isEmpty()) {
		delete m_alerts.takeAt(0);
	}
}

//-----------------------------------------------------------------------------
