/*
	SPDX-FileCopyrightText: 2010-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "alert_layer.h"

#include "action_manager.h"
#include "alert.h"

#include <QAction>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

AlertLayer::AlertLayer(QWidget* parent)
	: QWidget(parent)
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
	m_alerts.removeOne(alert);
}

//-----------------------------------------------------------------------------

void AlertLayer::dismissAlert()
{
	if (!m_alerts.isEmpty()) {
		for (int i = 0, count = m_alerts.count(); i < count; ++i) {
			if (m_alerts[i]->underMouse()) {
				delete m_alerts.takeAt(i);
				return;
			}
		}
		delete m_alerts.takeAt(0);
	}
}

//-----------------------------------------------------------------------------
