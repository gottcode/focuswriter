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

#ifndef LOCALE_DIALOG_H
#define LOCALE_DIALOG_H

#include <QDialog>
class QComboBox;

class LocaleDialog : public QDialog
{
	Q_OBJECT

public:
	LocaleDialog(QWidget* parent = 0);

	static void loadTranslator();

public slots:
	virtual void accept();

private:
	static QStringList findTranslations();

private:
	QComboBox* m_translations;

	static QString m_current;
	static QString m_path;
};

#endif
