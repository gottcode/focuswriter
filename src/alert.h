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

#ifndef ALERT_H
#define ALERT_H

#include <QWidget>
class QLabel;
class QTimeLine;
class QToolButton;

class Alert : public QWidget
{
	Q_OBJECT

public:
	Alert(const QString& text, const QStringList& details, QWidget* parent);

	void fadeIn();
	void setText(const QString& text, const QStringList& details);

	virtual bool eventFilter(QObject* watched, QEvent* event);

protected:
	virtual void enterEvent(QEvent* event);
	virtual void leaveEvent(QEvent* event);
	virtual void paintEvent(QPaintEvent* event);

private slots:
	void expanderToggled();
	void fadeInFinished();
	void fadeOut();

private:
	QToolButton* m_expander;
	QLabel* m_text;
	QString m_short_text;
	QString m_long_text;
	QTimeLine* m_fade_timer;
	bool m_expanded;
	bool m_under_mouse;
};

#endif
