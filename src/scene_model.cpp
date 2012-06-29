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

#include <QMimeData>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextEdit>

//-----------------------------------------------------------------------------

static QString f_scene_divider = QLatin1String("##");

//-----------------------------------------------------------------------------

SceneModel::SceneModel(QTextEdit* document, QObject* parent) :
	QAbstractListModel(parent),
	m_document(document)
{
	setSupportedDragActions(Qt::MoveAction);
	connect(m_document->document(), SIGNAL(blockCountChanged(int)), this, SLOT(invalidateScenes()));
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
	QTextBlock block = m_document->document()->findBlockByNumber(block_number).previous();
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
	Scene scene = { stats, text, QString(), block_number, true };
	m_scenes.insert(pos, scene);
	endInsertRows();

	// Make sure to update values
	invalidateScenes();
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

	// Make sure to update values
	invalidateScenes();
}

//-----------------------------------------------------------------------------

void SceneModel::updateScene(BlockStats* stats, int block_number, const QString& text)
{
	bool was_scene = stats->isScene();
	bool is_scene = text.startsWith(f_scene_divider);
	stats->setScene(is_scene || (block_number == 0));
	if (stats->isScene()) {
		QString scene_text = is_scene ? text.mid(f_scene_divider.length()).trimmed() : text;
		if (was_scene) {
			updateScene(stats, scene_text);
		} else {
			addScene(stats, block_number, scene_text);
		}
	} else if (was_scene) {
		removeScene(stats);
	} else {
		updateScene(block_number);
	}
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

void SceneModel::updateScene(int block_number)
{
	// Find first scene above block_number
	BlockStats* stats = 0;
	QTextBlock block = m_document->document()->findBlockByNumber(block_number);
	while (block.isValid()) {
		stats = static_cast<BlockStats*>(block.userData());
		if (stats && stats->isScene()) {
			break;
		}
		block = block.previous();
	}
	if (!stats || !stats->isScene()) {
		return;
	}

	// Find scene containing stats
	int pos = findSceneByStats(stats);
	if (pos == -1) {
		return;
	}

	// Modify scene
	m_scenes[pos].outdated = true;
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

		// Make sure the scene data is up-to-date
		if (scene.outdated) {
			scene.outdated = false;
			scene.block_number = -1;

			QStringList lines;
			QTextBlock block = m_document->document()->begin();
			while (block.isValid()) {
				BlockStats* stats = static_cast<BlockStats*>(block.userData());
				if (stats == scene.stats) {
					scene.block_number = block.blockNumber();
					lines += scene.text;
				} else if (!lines.isEmpty()) {
					if (stats && stats->isScene()) {
						break;
					} else {
						QString line = block.text().trimmed();
						if (!line.isEmpty()) {
							lines += line;
						}
						if (lines.count() == 3) {
							break;
						}
					}
				}
				block = block.next();
			}
			scene.display = lines.join(QLatin1String("\n")).trimmed();
		}

		if (role == Qt::DisplayRole) {
			result = scene.display;
		} else if (role == Qt::UserRole) {
			result = scene.block_number;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

Qt::ItemFlags SceneModel::flags(const QModelIndex& index) const
{
	return QAbstractListModel::flags(index) | (!index.isValid() ? Qt::ItemIsDropEnabled : Qt::ItemIsDragEnabled);
}

//-----------------------------------------------------------------------------

QMimeData* SceneModel::mimeData(const QModelIndexList& indexes) const
{
	// Encode list of scenes
	QByteArray bytes;
	QDataStream stream(&bytes, QIODevice::WriteOnly);
	QList<int> scenes;
	foreach (const QModelIndex& index, indexes) {
		scenes += index.row();
	}
	stream << scenes;

	// Return mime data object containing list
	QMimeData* data = new QMimeData();
	data->setData(mimeTypes().first(), bytes);
	return data;
}

//-----------------------------------------------------------------------------

QStringList SceneModel::mimeTypes() const
{
	return QStringList() << QLatin1String("application/x-fwscenelist");
}

//-----------------------------------------------------------------------------

int SceneModel::rowCount(const QModelIndex& parent) const
{
	return !parent.isValid() ? m_scenes.count() : 0;
}

//-----------------------------------------------------------------------------

Qt::DropActions SceneModel::supportedDropActions() const
{
	return Qt::MoveAction;
}

//-----------------------------------------------------------------------------

bool SceneModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	QString format = mimeTypes().first();
	if (!data || !data->hasFormat(format) || (action != Qt::MoveAction) || (column > 0) || parent.isValid()) {
		return false;
	}

	// Decode list of scenes
	QByteArray bytes = data->data(format);
	QDataStream stream(&bytes, QIODevice::ReadOnly);
	QList<int> scenes;
	stream >> scenes;
	if (scenes.isEmpty()) {
		return true;
	}
	qSort(scenes);

	// Don't allow scenes to be dropped on themselves
	if ((scenes.first() == row) || ((scenes.first() + 1) == row)) {
		return true;
	}

	// Copy text fragments of scenes
	QTextCursor cursor = m_document->textCursor();
	QList<QTextDocumentFragment> fragments;
	foreach (int scene, scenes) {
		selectScene(m_scenes.at(scene), cursor);
		fragments += cursor.selection();
	}

	// Find location in document to insert text fragments
	int position = 0;
	if ((row < m_scenes.size()) && (row > -1)) {
		const Scene& scene = m_scenes.at(row);
		QTextBlock block = m_document->document()->findBlockByNumber(scene.block_number);
		if (block.userData() == scene.stats) {
			position = block.position();
		} else {
			block = m_document->document()->begin();
			while (block.isValid()) {
				position = block.position();
				if (block.userData() == scene.stats) {
					break;
				}
				block = block.next();
			}
		}
	} else {
		cursor.movePosition(QTextCursor::End);
		if (cursor.block().text().length()) {
			cursor.insertBlock();
		}
		position = cursor.position();
	}

	// Start edit block by moving to start of dragged scenes
	cursor = m_document->textCursor();
	cursor.beginEditBlock();
	cursor.setPosition(position);

	// Insert text fragments; will indirectly create scenes
	foreach (const QTextDocumentFragment& fragment, fragments) {
		cursor.insertFragment(fragment);
		if (!cursor.atBlockStart()) {
			cursor.insertBlock();
		}
	}

	// Delete original fragments; will indirectly delete scenes
	int delta = 0;
	for (int i = scenes.count() - 1; i >= 0; --i) {
		selectScene(m_scenes.at(scenes.at(i)), cursor);
		delta += cursor.position();
		cursor.removeSelectedText();
		delta -= cursor.position();
	}

	// End edit block by moving to start of dropped scenes
	if (row > scenes.first()) {
		position -= delta;
	}
	cursor.setPosition(position);
	cursor.endEditBlock();
	m_document->setTextCursor(cursor);

	return true;
}

//-----------------------------------------------------------------------------

void SceneModel::setSceneDivider(const QString& divider)
{
	f_scene_divider = divider;
	f_scene_divider.replace(QLatin1String("\\t"), QLatin1String("\t"));
}

//-----------------------------------------------------------------------------

void SceneModel::invalidateScenes()
{
	int count = m_scenes.count();
	if (count == 0) {
		return;
	}

	for (int i = 0; i < count; ++i) {
		m_scenes[i].outdated = true;
	}
	emit dataChanged(index(0), index(count - 1));
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

void SceneModel::selectScene(const Scene& scene, QTextCursor& cursor) const
{
	// Select first block of scene
	QTextBlock block = cursor.document()->findBlockByNumber(scene.block_number);
	int position = block.position();
	if (block.userData() != scene.stats) {
		block = cursor.document()->begin();
		while (block.isValid()) {
			position = block.position();
			if (block.userData() == scene.stats) {
				break;
			}
			block = block.next();
		}
	}
	cursor.setPosition(position);
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

	// Select to last block of scene
	cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
	block = cursor.block();
	while (block.isValid()) {
		if (block.userData() && static_cast<BlockStats*>(block.userData())->isScene()) {
			break;
		}
		block = block.next();
		cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
	}
}

//-----------------------------------------------------------------------------
