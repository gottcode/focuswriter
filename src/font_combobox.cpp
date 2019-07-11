/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "font_combobox.h"

#include <QStringListModel>

//-----------------------------------------------------------------------------

FontComboBox::FontComboBox(QWidget* parent) :
	QComboBox(parent),
	m_system(QFontDatabase::Any)
{
	setEditable(false);

	m_font_model = new QStringListModel(this);
	updateModel();
	setModel(m_font_model);

	connect(this, &QComboBox::currentTextChanged, this, &FontComboBox::currentFamilyChanged);
}

//-----------------------------------------------------------------------------

void FontComboBox::setWritingSystem(QFontDatabase::WritingSystem system)
{
	m_system = system;
	updateModel();
}

//-----------------------------------------------------------------------------

void FontComboBox::setCurrentFont(const QFont& font)
{
	QString family = QFontInfo(font).family();
	QString foundryfamily = family + QLatin1String(" [");
	QStringList families = m_font_model->stringList();
	for (int i = 0; i < families.size(); ++i) {
		if (family == families.at(i) || families.at(i).startsWith(foundryfamily)) {
			setCurrentIndex(i);
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void FontComboBox::currentFamilyChanged(const QString& family)
{
	m_current.setFamily(family);
	emit currentFontChanged(m_current);
}

//-----------------------------------------------------------------------------

void FontComboBox::updateModel()
{
	QFontDatabase database;
	QStringList fonts = database.families(m_system);
	m_font_model->setStringList(fonts);
}

//-----------------------------------------------------------------------------
