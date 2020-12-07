/***********************************************************************
 *
 * Copyright (C) 2012-2020 Graeme Gott <graeme@gottcode.org>
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

#ifndef SYMBOLS_MODEL_H
#define SYMBOLS_MODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

class SymbolsModel : public QAbstractItemModel
{
	Q_OBJECT

private:
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

#endif
