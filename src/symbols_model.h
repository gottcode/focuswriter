/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SYMBOLS_MODEL_H
#define FOCUSWRITER_SYMBOLS_MODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>

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
		QList<Range> ranges;
	};
	typedef QList<Filter> FilterGroup;

public:
	explicit SymbolsModel(QObject* parent = nullptr);

	QStringList filters(int group) const;
	QStringList filterGroups() const;
	void setFilter(int group, int index);
	int symbolFilter(int group, char32_t unicode) const;
	QString symbolName(char32_t unicode) const;

	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QModelIndex index(char32_t unicode) const;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	static void setPath(const QString& path);

	friend QDataStream& operator>>(QDataStream& stream, SymbolsModel::Filter& filter);
	friend QDataStream& operator>>(QDataStream& stream, SymbolsModel::Filter::Range& range);

private:
	QList<char32_t> m_symbols;

	QHash<char32_t, QByteArray> m_names;
	QList<FilterGroup> m_groups;

	static QString m_path;
};

#endif // FOCUSWRITER_SYMBOLS_MODEL_H
