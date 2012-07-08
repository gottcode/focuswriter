/***********************************************************************
 *
 * Copyright (C) 2012 Graeme Gott <graeme@gottcode.org>
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

#ifndef SYMBOLS_DIALOG_H
#define SYMBOLS_DIALOG_H

class SymbolsModel;

#include <QDialog>
class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QSplitter;
class QTableView;
class QTableWidget;
class QTableWidgetItem;

class SymbolsDialog : public QDialog
{
	Q_OBJECT

public:
	SymbolsDialog(QWidget* parent = 0);

signals:
	void insertText(const QString& text);

public slots:
	void accept();
	void reject();

protected:
	void showEvent(QShowEvent* event);

private slots:
	void showFilter(QListWidgetItem* filter);
	void showGroup(int group);
	void symbolClicked(const QModelIndex& symbol);
	void recentSymbolClicked(QTableWidgetItem* symbol);

private:
	bool selectSymbol(quint32 unicode);
	void saveSettings();

private:
	SymbolsModel* m_model;

	QSplitter* m_contents;

	QComboBox* m_groups;
	QList<QListWidget*> m_filters;
	QTableView* m_view;

	class ElideLabel;
	ElideLabel* m_symbol_name;
	QLabel* m_symbol_code;

	QTableWidget* m_recent;
};

#endif
