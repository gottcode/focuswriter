/*
	SPDX-FileCopyrightText: 2009-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_THEME_MANAGER_H
#define FOCUSWRITER_THEME_MANAGER_H

class Theme;

#include <QDialog>
class QListWidget;
class QListWidgetItem;
class QSettings;
class QTabWidget;

class ThemeManager : public QDialog
{
	Q_OBJECT

public:
	explicit ThemeManager(QSettings& settings, QWidget* parent = nullptr);

Q_SIGNALS:
	void themeSelected(const Theme& theme);

protected:
	void hideEvent(QHideEvent* event) override;

private Q_SLOTS:
	void newTheme();
	void editTheme();
	void cloneTheme();
	void deleteTheme();
	void importTheme();
	void exportTheme();
	void currentThemeChanged(const QListWidgetItem* current);

private:
	QListWidgetItem* addItem(const QString& id, bool is_default, const QString& name);
	bool selectItem(const QString& id, bool is_default);
	void selectionChanged(bool is_default);

private:
	QTabWidget* m_tabs;
	QListWidget* m_default_themes;
	QListWidget* m_themes;
	QSettings& m_settings;
	QPushButton* m_clone_default_button;
	QPushButton* m_clone_button;
	QPushButton* m_edit_button;
	QPushButton* m_remove_button;
	QPushButton* m_export_button;
};

#endif // FOCUSWRITER_THEME_MANAGER_H
