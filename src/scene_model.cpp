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

#include "scene_model.h"

#include "block_stats.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

//-----------------------------------------------------------------------------

SceneModel::SceneModel(QTextEdit* document, QObject* parent) :
	QAbstractListModel(parent),
	m_document(document)
{
}

//-----------------------------------------------------------------------------

QModelIndex SceneModel::findScene(const QTextCursor& cursor) const
{
	// Find block stats for text cursor
	BlockStats* stats = 0;
	QTextBlock block = cursor.block();
	while (block.isValid()) {
		stats = static_cast<BlockStats*>(block.userData());
		if (stats && stats->isScene()) {
			break;
		}
		stats = 0;
		block = block.previous();
	}
	if (!stats) {
		return QModelIndex();
	}

	// Find block stats in scene list
	int pos = findSceneByStats(stats);
	return (pos != -1) ? index(pos) : QModelIndex();
}

//-----------------------------------------------------------------------------

void SceneModel::addScene(BlockStats* stats, int block_number, const QString& text)
{
	// Find previous scene in document
	BlockStats *before = 0, *check = 0;
	QTextBlock block = m_document->document()->findBlockByNumber(block_number);
	while (block.isValid()) {
		check = static_cast<BlockStats*>(block.userData());
		if (check && check->isScene()) {
			before = check;
			break;
		}
		block = block.previous();
	}

	// Find previous scene in list
	int pos = findSceneByStats(before) + 1;

	// Insert scene
	beginInsertRows(QModelIndex(), pos, pos);
	Scene scene = { stats, text };
	m_scenes.insert(pos, scene);
	endInsertRows();
}

//-----------------------------------------------------------------------------

void SceneModel::removeScene(BlockStats* stats)
{
	// Find scene containing stats
	int pos = findSceneByStats(stats);
	if (pos == -1) {
		return;
	}

	// Remove scene
	beginRemoveRows(QModelIndex(), pos, pos);
	m_scenes.removeAt(pos);
	endRemoveRows();
}

//-----------------------------------------------------------------------------

void SceneModel::updateScene(BlockStats* stats, const QString& text)
{
	// Find scene containing stats
	int pos = findSceneByStats(stats);
	if (pos == -1) {
		return;
	}

	// Modify scene
	m_scenes[pos].text = text;
	QModelIndex i = index(pos);
	emit dataChanged(i, i);
}

//-----------------------------------------------------------------------------

void SceneModel::clear()
{
	if (m_scenes.isEmpty()) {
		return;
	}

	beginRemoveRows(QModelIndex(), 0, m_scenes.count() - 1);
	m_scenes.clear();
	endRemoveRows();
}

//-----------------------------------------------------------------------------

QVariant SceneModel::data(const QModelIndex& index, int role) const
{
	QVariant result;

	if (index.row() < m_scenes.count()) {
		Scene scene = m_scenes.at(index.row());
		if (role == Qt::DisplayRole) {
			result = scene.text;
		} else if (role == Qt::UserRole) {
			QTextBlock block = m_document->document()->begin();
			while (block.isValid()) {
				if (block.userData() == scene.stats) {
					result = block.blockNumber();
				}
				block = block.next();
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

int SceneModel::rowCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? m_scenes.count() : 0;
}

//-----------------------------------------------------------------------------

int SceneModel::findSceneByStats(BlockStats* stats) const
{
	int pos = -1;
	for (int i = m_scenes.count() - 1; i >= 0; --i) {
		if (m_scenes.at(i).stats == stats) {
			pos = i;
			break;
		}
	}
	return pos;
}

//-----------------------------------------------------------------------------
