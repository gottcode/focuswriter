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

#ifndef SCENE_LIST_H
#define SCENE_LIST_H

class Document;

#include <QFrame>
class QLineEdit;
class QListView;
class QModelIndex;
class QSortFilterProxyModel;
class QToolButton;

class SceneList : public QFrame
{
	Q_OBJECT

public:
	SceneList(QWidget* parent = 0);
	~SceneList();

	bool scenesVisible() const;

	void setDocument(Document* document);

public slots:
	void hideScenes();
	void showScenes();

protected:
	void mouseMoveEvent(QMouseEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void resizeEvent(QResizeEvent* event);

private slots:
	void moveScenesDown();
	void moveScenesUp();
	void sceneSelected(const QModelIndex& index);
	void selectCurrentScene();
	void setFilter(const QString& filter);
	void toggleScenes();
	void updateShortcuts();

private:
	void moveSelectedScenes(int movement);

private:
	QAction* m_toggle_action;
	QToolButton* m_show_button;
	QListView* m_scenes;
	QLineEdit* m_filter;
	QToolButton* m_hide_button;
	QFrame* m_resizer;
	QSortFilterProxyModel* m_filter_model;
	Document* m_document;

	int m_width;
	QPoint m_mouse_current;
	bool m_resizing;
};

#endif
