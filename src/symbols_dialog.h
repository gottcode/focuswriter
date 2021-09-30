/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SYMBOLS_DIALOG_H
#define FOCUSWRITER_SYMBOLS_DIALOG_H

class ShortcutEdit;
class SymbolsModel;

#include <QDialog>
class QComboBox;
class QGraphicsView;
class QGraphicsSimpleTextItem;
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
	explicit SymbolsDialog(QWidget* parent = 0);

	void setInsertEnabled(bool enabled);
	void setPreviewFont(const QFont& font);

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
	void shortcutChanged();

private:
	bool selectSymbol(char32_t unicode);
	void saveSettings();

private:
	SymbolsModel* m_model;

	QSplitter* m_contents;

	QComboBox* m_groups;
	QList<QListWidget*> m_filters;
	QTableView* m_view;

	QGraphicsView* m_symbol_preview;
	QGraphicsSimpleTextItem* m_symbol_preview_item;
	ShortcutEdit* m_symbol_shortcut;
	class ElideLabel;
	ElideLabel* m_symbol_name;
	QLabel* m_symbol_code;

	QTableWidget* m_recent;

	QPushButton* m_insert_button;
};

#endif // FOCUSWRITER_SYMBOLS_DIALOG_H
