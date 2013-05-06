/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef FONT_COMBOBOX_H
#define FONT_COMBOBOX_H

#include <QComboBox>
#include <QFontDatabase>
class QStringListModel;

class FontComboBox : public QComboBox
{
	Q_OBJECT

public:
	FontComboBox(QWidget* parent = 0);

	QFont currentFont() const
	{
		return m_current;
	}

	QFontDatabase::WritingSystem writingSystem() const
	{
		return m_system;
	}

	void setWritingSystem(QFontDatabase::WritingSystem system);

public slots:
	void setCurrentFont(const QFont& font);

signals:
	void currentFontChanged(const QFont& font);

private slots:
	void currentFamilyChanged(const QString& family);

private:
	void updateModel();

private:
	QFont m_current;
	QStringListModel* m_font_model;
	QFontDatabase::WritingSystem m_system;
};

#endif
