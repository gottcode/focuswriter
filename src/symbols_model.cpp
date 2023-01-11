/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "symbols_model.h"

#include <QApplication>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QPalette>

#include <climits>

//-----------------------------------------------------------------------------

QDataStream& operator>>(QDataStream& stream, SymbolsModel::Filter::Range& range)
{
	stream >> range.start >> range.end;
	return stream;
}

QDataStream& operator>>(QDataStream& stream, SymbolsModel::Filter& filter)
{
	stream >> filter.name >> filter.size >> filter.ranges;
	return stream;
}

//-----------------------------------------------------------------------------

QString SymbolsModel::m_path;

//-----------------------------------------------------------------------------

SymbolsModel::SymbolsModel(QObject* parent)
	: QAbstractItemModel(parent)
{
	QFile file(m_path);
	if (!file.open(QFile::ReadOnly)) {
		return;
	}
	QByteArray data = qUncompress(file.readAll());
	file.close();

	QBuffer buffer(&data);
	if (!buffer.open(QBuffer::ReadOnly)) {
		return;
	}

	QDataStream stream(&buffer);
	stream.setVersion(QDataStream::Qt_6_2);
	stream >> m_names;
	stream >> m_groups;
	buffer.close();
}

//-----------------------------------------------------------------------------

QStringList SymbolsModel::filters(int group) const
{
	// Find group
	QStringList names;
	if ((group < 0) || (group >= m_groups.count())) {
		return names;
	}
	const FilterGroup& filters = m_groups.at(group);

	// List names of all filters in group
	for (const Filter& filter : filters) {
		names += QLatin1String(filter.name);
	}
	return names;
}

//-----------------------------------------------------------------------------

QStringList SymbolsModel::filterGroups() const
{
	static const QStringList groups{ tr("Blocks"), tr("Scripts") };
	return groups;
}

//-----------------------------------------------------------------------------

void SymbolsModel::setFilter(int group, int index)
{
	// Find filter
	if ((group < 0) || (group >= m_groups.count())) {
		return;
	}
	const FilterGroup& filters = m_groups.at(group);
	if ((index < 0) || (index >= filters.count())) {
		return;
	}
	const Filter& filter = filters.at(index);

	// Clear list of symbols
	beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
	m_symbols.resize(0);
	endRemoveRows();

	// Allocate space for symbols
	int padding = filter.size % 16;
	padding = padding ? (16 - padding) : 0;
	const int size = filter.size + padding;
	if (m_symbols.capacity() < size) {
		m_symbols.reserve(size);
	}

	// Create list of symbols
	beginInsertRows(QModelIndex(), 0, (size / 16) - 1);
	for (const Filter::Range& range : filter.ranges) {
		for (char32_t code = range.start; code <= range.end; ++code) {
			m_symbols.append(code);
		}
	}

	// Pad list of symbols to be multiple of 16
	for (int i = 0; i < padding; ++i) {
		m_symbols.append(UINT_MAX);
	}
	endInsertRows();
}

//-----------------------------------------------------------------------------

int SymbolsModel::symbolFilter(int group, char32_t unicode) const
{
	// Find group
	int index = -1;
	if ((group < 0) || (group >= m_groups.count())) {
		return index;
	}
	const FilterGroup& filters = m_groups.at(group);

	// Check for filter whose ranges contain symbol
	for (int i = 0, count = filters.count(); i < count; ++i) {
		const Filter& filter = filters.at(i);
		for (const Filter::Range& range : filter.ranges) {
			if ((range.start <= unicode) && (range.end >= unicode)) {
				index = i;
				break;
			}
		}
	}
	return index;
}

//-----------------------------------------------------------------------------

QString SymbolsModel::symbolName(char32_t unicode) const
{
	if ((unicode >= 0x3400 && unicode <= 0x4DBF)
			|| (unicode >= 0x4E00 && unicode <= 0x9FFF)
			|| (unicode >= 0x20000 && unicode <= 0x2A6DF)
			|| (unicode >= 0x2A700 && unicode <= 0x2B81F)) {
		return QLatin1String("CJK UNIFIED IDEOGRAPH-") + QString::number(unicode, 16).toUpper();
	} else if (unicode >= 0xAC00 && unicode <= 0xD7AF) {
		// Hangul character name algorithm from section 3.12 of Unicode standard
		static const int SBase = 0xAC00;
		static const int LCount = 19;
		static const int VCount = 21;
		static const int TCount = 28;
		static const int NCount = (VCount * TCount);
		static const int SCount = (LCount * NCount);

		static const char JAMO_L_TABLE[][4] =
		{
			"G", "GG", "N", "D", "DD", "R", "M", "B", "BB",
			"S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H"
		};

		static const char JAMO_V_TABLE[][4] =
		{
			"A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O",
			"WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI",
			"YU", "EU", "YI", "I"
		};

		static const char JAMO_T_TABLE[][4] =
		{
			"", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM",
			"LB", "LS", "LT", "LP", "LH", "M", "B", "BS",
			"S", "SS", "NG", "J", "C", "K", "T", "P", "H"
		};

		const int SIndex = unicode - SBase;
		if (SIndex < 0 || SIndex >= SCount) {
			return QString();
		}

		const int LIndex = SIndex / NCount;
		const int VIndex = (SIndex % NCount) / TCount;
		const int TIndex = SIndex % TCount;

		return QLatin1String("HANGUL SYLLABLE ") + QLatin1String(JAMO_L_TABLE[LIndex]) +
				QLatin1String(JAMO_V_TABLE[VIndex]) + QLatin1String(JAMO_T_TABLE[TIndex]);
	} else {
		return QLatin1String(m_names[unicode]);
	}
}

//-----------------------------------------------------------------------------

int SymbolsModel::columnCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? 16 : 0;
}

//-----------------------------------------------------------------------------

QVariant SymbolsModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	const char32_t unicode = index.internalId();
	const bool printable = QChar(unicode).isPrint();
	switch (role) {
	case Qt::BackgroundRole:
		return printable ? QVariant() : QApplication::palette().button();
	case Qt::DisplayRole:
		return printable ? QString::fromUcs4(&unicode, 1) : QString();
	case Qt::TextAlignmentRole:
		return Qt::AlignCenter;
	default:
		return QVariant();
	}
}

//-----------------------------------------------------------------------------

Qt::ItemFlags SymbolsModel::flags(const QModelIndex& index) const
{
	if (!index.isValid() || (index.internalId() > 0x10FFFF)) {
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}

//-----------------------------------------------------------------------------

QModelIndex SymbolsModel::index(char32_t unicode) const
{
	const int index = m_symbols.indexOf(unicode);
	if (index != -1) {
		return createIndex(index / 16, index % 16, unicode);
	} else {
		return QModelIndex();
	}
}

//-----------------------------------------------------------------------------

QModelIndex SymbolsModel::index(int row, int column, const QModelIndex& parent) const
{
	const int pos = (row * 16) + column;
	return (!parent.isValid() && (pos < m_symbols.count())) ? createIndex(row, column, m_symbols.at(pos)) : QModelIndex();
}

//-----------------------------------------------------------------------------

QModelIndex SymbolsModel::parent(const QModelIndex&) const
{
	return QModelIndex();
}

//-----------------------------------------------------------------------------

int SymbolsModel::rowCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? (m_symbols.count() / 16) : 0;
}

//-----------------------------------------------------------------------------

void SymbolsModel::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------
