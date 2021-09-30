/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SYMBOLS_MODEL_H
#define FOCUSWRITER_SYMBOLS_MODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

class SymbolsModel : public QAbstractItemModel
{
	Q_OBJECT

	struct Filter
	{
		struct Range
		{
			char32_t start;
			char32_t end;
		};

		QByteArray name;
		char32_t size;
		QVector<Range> ranges;
	};
	typedef QVector<Filter> FilterGroup;

public:
	SymbolsModel(QObject* parent = 0);

	QStringList filters(int group) const;
	QStringList filterGroups() const;
	void setFilter(int group, int index);
	int symbolFilter(int group, char32_t unicode) const;
	QString symbolName(char32_t unicode) const;

	int columnCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QModelIndex index(char32_t unicode) const;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex& child) const;
	int rowCount(const QModelIndex& parent = QModelIndex()) const;

	static void setData(const QStringList& datadirs);

	friend QDataStream& operator>>(QDataStream& stream, SymbolsModel::Filter& filter);
	friend QDataStream& operator>>(QDataStream& stream, SymbolsModel::Filter::Range& range);

private:
	QVector<char32_t> m_symbols;

	QHash<char32_t, QByteArray> m_names;
	QVector<FilterGroup> m_groups;

	static QString m_path;
};

#endif // FOCUSWRITER_SYMBOLS_MODEL_H
