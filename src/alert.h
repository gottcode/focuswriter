/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2018 Graeme Gott <graeme@gottcode.org>
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
class QGraphicsOpacityEffect;
class QLabel;
class QTimeLine;
class QToolButton;

class Alert : public QWidget
{
	Q_OBJECT

public:
	enum Icon {
		NoIcon = 0,
		Information,
		Warning,
		Critical,
		Question
	};

	Alert(QWidget* parent = 0);
	Alert(Icon icon, const QString& text, const QStringList& details, bool expandable, QWidget* parent = 0);

	bool underMouse() const;

	void fadeIn();
	void setExpandable(bool expandable);
	void setIcon(Icon icon);
	void setIcon(const QPixmap& pixmap);
	void setText(const QString& text, const QStringList& details);

	bool eventFilter(QObject* watched, QEvent* event);

protected:
	void enterEvent(QEvent* event);
	void leaveEvent(QEvent* event);
	void paintEvent(QPaintEvent* event);

private slots:
	void expanderToggled();
	void fadeInFinished();
	void fadeOut();

private:
	void init();

private:
	QToolButton* m_expander;
	QLabel* m_pixmap;
	QLabel* m_text;
	QString m_short_text;
	QString m_long_text;
	QTimeLine* m_fade_timer;
	QGraphicsOpacityEffect* m_fade_effect;
	bool m_expanded;
	bool m_always_expanded;
	bool m_under_mouse;
};

inline bool Alert::underMouse() const
{
	return m_under_mouse;
}

#endif
