/*
	SPDX-FileCopyrightText: 2010-2012 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_ALERT_LAYER_H
#define FOCUSWRITER_ALERT_LAYER_H

class Alert;

#include <QList>
#include <QMessageBox>
#include <QWidget>
class QVBoxLayout;

class AlertLayer : public QWidget
{
	Q_OBJECT

public:
	explicit AlertLayer(QWidget* parent);

public Q_SLOTS:
	void addAlert(Alert* alert);

private Q_SLOTS:
	void alertDestroyed(QObject* alert);
	void dismissAlert();

private:
	QList<Alert*> m_alerts;
	QVBoxLayout* m_alerts_layout;
};

#endif // FOCUSWRITER_ALERT_LAYER_H
