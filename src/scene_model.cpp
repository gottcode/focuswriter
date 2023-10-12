/*
	SPDX-FileCopyrightText: 2012-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "scene_model.h"

#include "block_stats.h"

#include <QMimeData>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextEdit>

#include <algorithm>

//-----------------------------------------------------------------------------

static QString f_scene_divider = QLatin1String("##");
static QList<SceneModel*> f_scene_models;

//-----------------------------------------------------------------------------

SceneModel::SceneModel(QTextEdit* document, QObject* parent)
	: QAbstractListModel(parent)
	, m_document(document)
	, m_updates(0)
{
	connect(m_document->document(), &QTextDocument::blockCountChanged, this, &SceneModel::invalidateScenes);

	f_scene_models.append(this);
}

//-----------------------------------------------------------------------------

SceneModel::~SceneModel()
{
	f_scene_models.removeAll(this);
}

//-----------------------------------------------------------------------------

QModelIndex SceneModel::findScene(const QTextCursor& cursor) const
{
	// Find block stats for text cursor
	const BlockStats* stats = nullptr;
	for (QTextBlock block = cursor.block(); block.isValid(); block = block.previous()) {
		stats = static_cast<BlockStats*>(block.userData());
		if (stats && stats->isScene()) {
			break;
		}
		stats = nullptr;
	}
	if (!stats) {
		return QModelIndex();
	}

	// Find block stats in scene list
	const int pos = findSceneByStats(stats);
	return (pos != -1) ? index(pos) : QModelIndex();
}

//-----------------------------------------------------------------------------

void SceneModel::moveScenes(QList<int> scenes, int row)
{
	// Make sure scenes are ordered correctly
	if (scenes.isEmpty()) {
		return;
	}
	std::sort(scenes.begin(), scenes.end());

	// Copy text fragments of scenes
	QTextCursor cursor = m_document->textCursor();
	QList<QTextDocumentFragment> fragments;
	for (int scene : std::as_const(scenes)) {
		selectScene(m_scenes.at(scene), cursor);
		fragments += cursor.selection();
	}

	// Find location in document to insert text fragments
	int position = 0;
	if ((row < m_scenes.count()) && (row > -1)) {
		const Scene& scene = m_scenes.at(row);
		const QTextBlock block = m_document->document()->findBlockByNumber(scene.block_number);
		if (block.userData() == scene.stats) {
			position = block.position();
		} else {
			for (QTextBlock block = m_document->document()->begin(); block.isValid(); block = block.next()) {
				position = block.position();
				if (block.userData() == scene.stats) {
					break;
				}
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

	// Make sure inserted text begins with divider
	if (!fragments.constFirst().toPlainText().startsWith(f_scene_divider)) {
		cursor.insertText(f_scene_divider + "\n");
	}

	// Insert text fragments; will indirectly create scenes
	for (const QTextDocumentFragment& fragment : std::as_const(fragments)) {
		cursor.insertFragment(fragment);
		if (!cursor.atBlockStart()) {
			cursor.insertBlock();
		}
	}

	// Make sure inserted text ends with divider
	if (!cursor.atEnd() && !cursor.block().text().startsWith(f_scene_divider)) {
		cursor.insertText(f_scene_divider + "\n");
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
	if (row > scenes.constFirst()) {
		position -= delta;
	}
	cursor.setPosition(position);
	cursor.endEditBlock();
	m_document->setTextCursor(cursor);
}

//-----------------------------------------------------------------------------

void SceneModel::removeScene(const BlockStats* stats)
{
	// Find scene containing stats
	const int pos = findSceneByStats(stats);
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

void SceneModel::removeAllScenes()
{
	if (m_scenes.isEmpty()) {
		return;
	}

	beginRemoveRows(QModelIndex(), 0, m_scenes.count() - 1);
	m_scenes.clear();
	endRemoveRows();
}

//-----------------------------------------------------------------------------

void SceneModel::updateScene(BlockStats* stats, const QTextBlock& block)
{
	// Flag scenes out-of-date
	if (m_updates < 1) {
		m_updates = -1;
		return;
	}

	QString text = block.text();
	const bool was_scene = stats->isScene();
	const bool is_scene = !f_scene_divider.isEmpty() && text.startsWith(f_scene_divider);
	stats->setScene(is_scene || (block.blockNumber() == 0));
	if (stats->isScene()) {
		// Add or update scene divider block
		text = is_scene ? text.mid(f_scene_divider.length()).trimmed() : text;
		if (was_scene) {
			updateScene(stats, text);
		} else {
			addScene(stats, block, text);
		}
	} else if (was_scene) {
		removeScene(stats);
	} else {
		updateScene(block);
	}
}

//-----------------------------------------------------------------------------

void SceneModel::setUpdatesBlocked(bool blocked)
{
	if (!blocked) {
		resetScenes();
	}
	m_updates = !blocked;
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
			for (QTextBlock block = m_document->document()->begin(); block.isValid(); block = block.next()) {
				const BlockStats* stats = static_cast<BlockStats*>(block.userData());
				if (stats == scene.stats) {
					scene.block_number = block.blockNumber();
					lines += scene.text;
				} else if (!lines.isEmpty()) {
					if (stats && stats->isScene()) {
						break;
					} else {
						const QString line = block.text().trimmed();
						if (!line.isEmpty()) {
							lines += line;
						}
						if (lines.count() == 3) {
							break;
						}
					}
				}
			}
			scene.display = lines.join(QLatin1String("\n")).trimmed();
		}

		if (role == Qt::DisplayRole) {
			result = scene.display;
		} else if (role == Qt::UserRole) {
			result = scene.block_number;
		} else if (role == Qt::TextAlignmentRole) {
			result = int(Qt::AlignLeading | Qt::AlignTop);
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

bool SceneModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	const QString format = mimeTypes().constFirst();
	if (!data || !data->hasFormat(format) || (action != Qt::MoveAction) || (column > 0) || parent.isValid()) {
		return false;
	}

	// Decode list of scenes
	QByteArray bytes = data->data(format);
	QDataStream stream(&bytes, QIODevice::ReadOnly);
	QList<int> scenes;
	stream >> scenes;

	moveScenes(scenes, row);

	return true;
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
	for (const QModelIndex& index : indexes) {
		scenes += index.row();
	}
	stream << scenes;

	// Return mime data object containing list
	QMimeData* data = new QMimeData();
	data->setData(mimeTypes().constFirst(), bytes);
	return data;
}

//-----------------------------------------------------------------------------

QStringList SceneModel::mimeTypes() const
{
	static const QStringList types{ QStringLiteral("application/x-fwscenelist") };
	return types;
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

void SceneModel::setSceneDivider(const QString& divider)
{
	if (f_scene_divider == divider) {
		return;
	}

	f_scene_divider = divider;
	f_scene_divider.replace(QLatin1String("\\t"), QLatin1String("\t"));

	for (SceneModel* model : std::as_const(f_scene_models)) {
		model->resetScenes();
	}
}

//-----------------------------------------------------------------------------

void SceneModel::selectScene()
{
	// Make sure scenes are up-to-date
	if (m_updates == -1) {
		resetScenes();
	}

	QTextCursor cursor = m_document->textCursor();
	cursor.clearSelection();

	// Select to first block of scene
	cursor.movePosition(QTextCursor::StartOfBlock);
	for (QTextBlock block = cursor.block(); block.isValid(); block = block.previous()) {
		const BlockStats* stats = static_cast<BlockStats*>(block.userData());
		if (stats && stats->isScene()) {
			break;
		}
		cursor.movePosition(QTextCursor::StartOfBlock);
		cursor.movePosition(QTextCursor::PreviousBlock);
	}
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

	// Select to last block of scene
	cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
	for (QTextBlock block = cursor.block(); block.isValid(); block = block.next()) {
		const BlockStats* stats = static_cast<BlockStats*>(block.userData());
		if (stats && stats->isScene()) {
			break;
		}
		cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
	}

	m_document->setTextCursor(cursor);
}

//-----------------------------------------------------------------------------

void SceneModel::invalidateScenes()
{
	const int count = m_scenes.count();
	if (count == 0) {
		return;
	}

	for (int i = 0; i < count; ++i) {
		m_scenes[i].outdated = true;
	}
	Q_EMIT dataChanged(index(0), index(count - 1));
}

//-----------------------------------------------------------------------------

void SceneModel::addScene(const BlockStats* stats, const QTextBlock& block, const QString& text)
{
	// Find previous scene in document
	const BlockStats* before = nullptr;
	for (QTextBlock previous = block.previous(); previous.isValid(); previous = previous.previous()) {
		const BlockStats* check = static_cast<BlockStats*>(previous.userData());
		if (check && check->isScene()) {
			before = check;
			break;
		}
	}

	// Find previous scene in list
	const int pos = findSceneByStats(before) + 1;

	// Insert scene
	beginInsertRows(QModelIndex(), pos, pos);
	m_scenes.insert(pos, Scene{ stats, text, QString(), block.blockNumber(), true });
	endInsertRows();

	// Make sure to update values
	invalidateScenes();
}

//-----------------------------------------------------------------------------

int SceneModel::findSceneByStats(const BlockStats* stats) const
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

void SceneModel::resetScenes()
{
	// Remove all current scenes
	removeAllScenes();

	// Check all blocks for new scenes
	QList<Scene> scenes;
	for (QTextBlock block = m_document->document()->begin(); block.isValid(); block = block.next()) {
		BlockStats* stats = static_cast<BlockStats*>(block.userData());
		if (stats) {
			// Check if block is a scene
			QString text = block.text();
			const bool is_scene = !f_scene_divider.isEmpty() && text.startsWith(f_scene_divider);
			stats->setScene(is_scene || (block.blockNumber() == 0));

			// Add scene
			if (stats->isScene()) {
				text = is_scene ? text.mid(f_scene_divider.length()).trimmed() : text;
				scenes.append(Scene{ stats, text, QString(), block.blockNumber(), true });
			}
		}
	}

	// Add all found scenes
	if (!scenes.isEmpty()) {
		beginInsertRows(QModelIndex(), 0, scenes.count() - 1);
		m_scenes = scenes;
		endInsertRows();
	}

	// Remove out-of-date flag
	if (m_updates == -1) {
		m_updates = 0;
	}
}

//-----------------------------------------------------------------------------

void SceneModel::selectScene(const Scene& scene, QTextCursor& cursor) const
{
	// Select first block of scene
	const QTextBlock block = cursor.document()->findBlockByNumber(scene.block_number);
	int position = block.position();
	if (block.userData() != scene.stats) {
		for (QTextBlock block = cursor.document()->begin(); block.isValid(); block = block.next()) {
			position = block.position();
			if (block.userData() == scene.stats) {
				break;
			}
		}
	}
	cursor.setPosition(position);
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

	// Select to last block of scene
	cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
	for (QTextBlock block = cursor.block(); block.isValid(); block = block.next()) {
		const BlockStats* stats = static_cast<BlockStats*>(block.userData());
		if ((stats && stats->isScene()) || block.text().startsWith(f_scene_divider)) {
			break;
		}
		cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
	}
}

//-----------------------------------------------------------------------------

void SceneModel::updateScene(const BlockStats* stats, const QString& text)
{
	// Find scene containing stats
	const int pos = findSceneByStats(stats);
	if (pos == -1) {
		return;
	}

	// Modify scene
	m_scenes[pos].text = text;
	m_scenes[pos].outdated = true;
	const QModelIndex i = index(pos);
	Q_EMIT dataChanged(i, i);
}

//-----------------------------------------------------------------------------

void SceneModel::updateScene(const QTextBlock& block)
{
	// Find first scene above block
	const BlockStats* stats = nullptr;
	int count = 0;
	for (QTextBlock check = block; check.isValid(); check = check.previous()) {
		stats = static_cast<BlockStats*>(check.userData());
		if (stats && stats->isScene()) {
			break;
		}
		++count;
		if (count == 3) {
			return;
		}
	}
	if (!stats || !stats->isScene()) {
		return;
	}

	// Find scene containing stats
	const int pos = findSceneByStats(stats);
	if (pos == -1) {
		return;
	}

	// Modify scene
	m_scenes[pos].outdated = true;
	const QModelIndex i = index(pos);
	Q_EMIT dataChanged(i, i);
}

//-----------------------------------------------------------------------------
