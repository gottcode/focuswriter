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
class QTextCursor;
class QTextEdit;

class SceneModel : public QAbstractListModel
{
	Q_OBJECT

	struct Scene
	{
		BlockStats* stats;
		QString text;
	};

public:
	SceneModel(QTextEdit* document, QObject* parent = 0);

	QModelIndex findScene(const QTextCursor& cursor) const;

	void addScene(BlockStats* stats, int block_number, const QString& text);
	void removeScene(BlockStats* stats);
	void updateScene(BlockStats* stats, const QString& text);

	void clear();

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex& parent) const;

private:
	int findSceneByStats(BlockStats* stats) const;

private:
	QList<Scene> m_scenes;
	QTextEdit* m_document;
};

#endif
