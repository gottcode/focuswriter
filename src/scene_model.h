/*
	SPDX-FileCopyrightText: 2012 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SCENE_MODEL_H
#define FOCUSWRITER_SCENE_MODEL_H

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
	explicit SceneModel(QTextEdit* document, QObject* parent = nullptr);
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

#endif // FOCUSWRITER_SCENE_MODEL_H
