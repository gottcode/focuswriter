/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "action_manager.h"
#include "alert.h"

#include <QAction>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

AlertLayer::AlertLayer(QWidget* parent) :
	QWidget(parent)
{
	setMaximumWidth(4 * logicalDpiX());
	m_alerts_layout = new QVBoxLayout(this);
	m_alerts_layout->setContentsMargins(0, 0, 0, 0);

	QAction* action = new QAction(tr("Dismiss Alert"), this);
	action->setShortcut(tr("Ctrl+D"));
	connect(action, &QAction::triggered, this, &AlertLayer::dismissAlert);
	addAction(action);
	ActionManager::instance()->addAction("DismissAlert", action);
}

//-----------------------------------------------------------------------------

void AlertLayer::addAlert(Alert* alert)
{
	alert->setParent(this);
	m_alerts.append(alert);
	m_alerts_layout->addWidget(alert);
	connect(alert, &Alert::destroyed, this, &AlertLayer::alertDestroyed);
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
		for (int i = 0; i < m_alerts.count(); ++i) {
			if (m_alerts[i]->underMouse()) {
				delete m_alerts.takeAt(i);
				return;
			}
		}
		delete m_alerts.takeAt(0);
	}
}

//-----------------------------------------------------------------------------
