/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2014 Graeme Gott <graeme@gottcode.org>
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

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

class Theme;

#include <QDialog>
class QListWidget;
class QListWidgetItem;
class QSettings;

class ThemeManager : public QDialog
{
	Q_OBJECT

public:
	ThemeManager(QSettings& settings, QWidget* parent = 0);

signals:
	void themeSelected(const Theme& theme);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void addTheme();
	void modifyTheme();
	void cloneTheme();
	void removeTheme();
	void importTheme();
	void exportTheme();
	void currentThemeChanged(QListWidgetItem* current);

private:
	void addItem(const QString& name);

private:
	QListWidget* m_themes;
	QSettings& m_settings;
	QPushButton* m_remove_button;
};

#endif
