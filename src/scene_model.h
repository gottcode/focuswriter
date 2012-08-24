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

#ifndef SCENE_MODEL_H
#define SCENE_MODEL_H

class BlockStats;

#include <QAbstractListModel>
#include <QList>
class QTextBlock;
class QTextCursor;
class QTextEdit;

class SceneModel : public QAbstractListModel
{
	Q_OBJECT

	struct Scene
	{
		BlockStats* stats;
		QString text;
		QString display;
		int block_number;
		bool outdated;
	};

public:
	SceneModel(QTextEdit* document, QObject* parent = 0);
	~SceneModel();

	QModelIndex findScene(const QTextCursor& cursor) const;
	void moveScenes(QList<int> scenes, int row);
	void removeScene(BlockStats* stats);
	void removeAllScenes();
	void updateScene(BlockStats* stats, const QTextBlock& block);
	void setUpdatesBlocked(bool blocked);

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
	Qt::ItemFlags flags(const QModelIndex& index) const;
	QMimeData* mimeData(const QModelIndexList& indexes) const;
	QStringList mimeTypes() const;
	int rowCount(const QModelIndex& parent) const;
	Qt::DropActions supportedDropActions() const;

	static void setSceneDivider(const QString& divider);

public slots:
	void selectScene();

private slots:
	void invalidateScenes();

private:
	void addScene(BlockStats* stats, const QTextBlock& block, const QString& text);
	int findSceneByStats(BlockStats* stats) const;
	void resetScenes();
	void selectScene(const Scene& scene, QTextCursor& cursor) const;
	void updateScene(BlockStats* stats, const QString& text);
	void updateScene(const QTextBlock& block);

private:
	QList<Scene> m_scenes;
	QTextEdit* m_document;
	int m_updates;
};

#endif
