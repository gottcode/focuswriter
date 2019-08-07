/***********************************************************************
 *
 * Copyright (C) 2012, 2014, 2018, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "scene_list.h"

#include "action_manager.h"
#include "document.h"
#include "scene_model.h"

#include <QAction>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMimeData>
#include <QMouseEvent>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTextBlock>
#include <QTextEdit>
#include <QToolButton>

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

namespace
{

class SceneDelegate : public QStyledItemDelegate
{
public:
	SceneDelegate(QObject* parent) :
		QStyledItemDelegate(parent)
	{
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

QSize SceneDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem opt = option;
	initStyleOption(&opt, index);
	const QWidget* widget = opt.widget;
	const QStyle* style = widget ? widget->style() : QApplication::style();

	QSize size = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);
#if !defined(Q_OS_MAC)
	int margin = style->pixelMetric(QStyle::PM_FocusFrameVMargin, &opt, widget);
#else
	int margin = 0;
#endif
	int height = opt.fontMetrics.height() * 3;
	size.setHeight(margin + height);
	return size;
}

}

//-----------------------------------------------------------------------------

SceneList::SceneList(QWidget* parent) :
	QFrame(parent),
	m_document(0),
	m_resizing(false)
{
	m_width = qBound(0, QSettings().value("SceneList/Width", (int)std::lround(3.5 * logicalDpiX())).toInt(), maximumWidth());

	// Configure sidebar
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setAutoFillBackground(true);
	setPalette(QApplication::palette());

	// Create actions for moving scenes
	QAction* action = new QAction(tr("Move Scenes Down"), this);
	action->setShortcut(tr("Ctrl+Shift+Down"));
	connect(action, &QAction::triggered, this, &SceneList::moveScenesDown);
	addAction(action);
	ActionManager::instance()->addAction("MoveScenesDown", action);

	action = new QAction(tr("Move Scenes Up"), this);
	action->setShortcut(tr("Ctrl+Shift+Up"));
	connect(action, &QAction::triggered, this, &SceneList::moveScenesUp);
	addAction(action);
	ActionManager::instance()->addAction("MoveScenesUp", action);

	// Create button to show scenes
	m_show_button = new QToolButton(this);
	m_show_button->setAutoRaise(true);
	m_show_button->setArrowType(Qt::RightArrow);
	m_show_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
	connect(m_show_button, &QToolButton::clicked, this, &SceneList::showScenes);

	// Create button to hide scenes
	m_hide_button = new QToolButton(this);
	m_hide_button->setAutoRaise(true);
	m_hide_button->setArrowType(Qt::LeftArrow);
	m_hide_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
	connect(m_hide_button, &QToolButton::clicked, this, &SceneList::hideScenes);

	// Create action for toggling scenes
	m_toggle_action = new QAction(tr("Toggle Scene List"), this);
	m_toggle_action->setShortcut(tr("Shift+F4"));
	connect(m_toggle_action, &QAction::changed, this, &SceneList::updateShortcuts);
	connect(m_toggle_action, &QAction::triggered, this, &SceneList::toggleScenes);
	ActionManager::instance()->addAction("ToggleScenes", m_toggle_action);
	updateShortcuts();
	parent->addAction(m_toggle_action);

	// Create scene view
	m_filter_model = new QSortFilterProxyModel(this);
	m_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);

	m_scenes = new QListView(this);
	m_scenes->setAlternatingRowColors(true);
	m_scenes->setDragEnabled(true);
	m_scenes->setDragDropMode(QAbstractItemView::InternalMove);
	m_scenes->setDropIndicatorShown(true);
	m_scenes->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_scenes->setItemDelegate(new SceneDelegate(m_scenes));
	m_scenes->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_scenes->setUniformItemSizes(true);
	m_scenes->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_scenes->setWordWrap(true);
	m_scenes->viewport()->setAcceptDrops(true);
	m_scenes->setModel(m_filter_model);
	m_scenes->show();
	setFocusProxy(m_scenes);
	setFocusPolicy(Qt::StrongFocus);

	// Create filter widget
	m_filter = new QLineEdit(this);
	m_filter->setPlaceholderText(tr("Filter"));
	connect(m_filter, &QLineEdit::textChanged, this, &SceneList::setFilter);

	// Create widget for resizing
	m_resizer = new QFrame(this);
	m_resizer->setCursor(Qt::SizeHorCursor);
	m_resizer->setFrameStyle(QFrame::VLine | QFrame::Sunken);
	m_resizer->setToolTip(tr("Resize scene list"));

	// Lay out widgets
	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(0, 1);
	layout->addWidget(m_show_button, 0, 0, 2, 1);
	layout->addWidget(m_hide_button, 0, 1, 2, 1);
	layout->addWidget(m_scenes, 0, 2);
	layout->addWidget(m_filter, 1, 2);
	layout->addWidget(m_resizer, 0, 3, 2, 1);

	// Start collapsed
	hideScenes();
}

//-----------------------------------------------------------------------------

SceneList::~SceneList()
{
	QSettings().setValue("SceneList/Width", m_width);
}

//-----------------------------------------------------------------------------

bool SceneList::scenesVisible() const
{
	return m_scenes->isVisible();
}

//-----------------------------------------------------------------------------

void SceneList::setDocument(Document* document)
{
	if (m_document) {
		disconnect(m_document->text(), &QTextEdit::cursorPositionChanged, this, &SceneList::selectCurrentScene);
	}
	m_document = 0;

	m_scenes->clearSelection();
	m_filter->clear();
	m_filter_model->setSourceModel(document->sceneModel());

	m_document = document;
	if (m_document && scenesVisible()) {
		m_scenes->setDragDropMode(!m_document->text()->isReadOnly() ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
		m_document->sceneModel()->setUpdatesBlocked(false);
		connect(m_document->text(), &QTextEdit::cursorPositionChanged, this, &SceneList::selectCurrentScene);
		selectCurrentScene();
	}
}

//-----------------------------------------------------------------------------

void SceneList::hideScenes()
{
	if (m_document) {
		disconnect(m_scenes->selectionModel(), &QItemSelectionModel::currentChanged, this, &SceneList::sceneSelected);
		m_document->sceneModel()->setUpdatesBlocked(true);
		disconnect(m_document->text(), &QTextEdit::cursorPositionChanged, this, &SceneList::selectCurrentScene);
	}

	m_show_button->show();

	m_hide_button->hide();
	m_scenes->hide();
	m_filter->hide();
	m_resizer->hide();

	setMinimumWidth(0);
	setMaximumWidth(minimumSizeHint().width());

	m_filter->clear();

	hide();

	if (m_document) {
		m_document->text()->setFocus();
	}
}

//-----------------------------------------------------------------------------

void SceneList::showScenes()
{
	show();

	m_hide_button->show();
	m_scenes->show();
	m_filter->show();
	m_resizer->show();

	m_show_button->hide();

	setMinimumWidth(std::lround(1.5 * logicalDpiX()));
	setMaximumWidth(m_width);

	if (m_document) {
		m_scenes->setDragDropMode(!m_document->text()->isReadOnly() ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
		m_document->sceneModel()->setUpdatesBlocked(false);
		connect(m_document->text(), &QTextEdit::cursorPositionChanged, this, &SceneList::selectCurrentScene);
		selectCurrentScene();
		connect(m_scenes->selectionModel(), &QItemSelectionModel::currentChanged, this, &SceneList::sceneSelected);
	}

	m_scenes->setFocus();
}

//-----------------------------------------------------------------------------

void SceneList::mouseMoveEvent(QMouseEvent* event)
{
	if (m_resizing) {
		int delta = event->pos().x() - m_mouse_current.x();
		m_mouse_current = event->pos();

		m_width += delta;
		m_width = std::max(minimumWidth(), m_width);
		setMaximumWidth(m_width);

		event->accept();
	} else {
		QFrame::mouseMoveEvent(event);
	}
}

//-----------------------------------------------------------------------------

void SceneList::mousePressEvent(QMouseEvent* event)
{
	if (scenesVisible() &&
			(event->button() == Qt::LeftButton) &&
			(event->pos().x() >= m_resizer->mapToParent(m_resizer->rect().topLeft()).x())) {
		m_width = width();
		m_mouse_current = event->pos();
		m_resizing = true;

		event->accept();
	} else {
		QFrame::mousePressEvent(event);
	}
}

//-----------------------------------------------------------------------------

void SceneList::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_resizing = false;
	}
	QFrame::mouseReleaseEvent(event);
}

//-----------------------------------------------------------------------------

void SceneList::resizeEvent(QResizeEvent* event)
{
	m_scenes->scrollTo(m_scenes->currentIndex());
	QFrame::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void SceneList::moveScenesDown()
{
	moveSelectedScenes(1);
}

//-----------------------------------------------------------------------------

void SceneList::moveScenesUp()
{
	moveSelectedScenes(-1);
}

//-----------------------------------------------------------------------------

void SceneList::sceneSelected(const QModelIndex& index)
{
	if (!m_document || !scenesVisible()) {
		return;
	}

	if (index.isValid()) {
		int block_number = index.data(Qt::UserRole).toInt();
		QTextBlock block = m_document->text()->document()->findBlockByNumber(block_number);
		QTextCursor cursor = m_document->text()->textCursor();
		cursor.setPosition(block.position());
		m_document->text()->setTextCursor(cursor);
		m_document->centerCursor(true);
	}
}

//-----------------------------------------------------------------------------

void SceneList::selectCurrentScene()
{
	if (!m_document || !scenesVisible()) {
		return;
	}

	QModelIndex index = m_document->sceneModel()->findScene(m_document->text()->textCursor());
	if (index.isValid()) {
		index = m_filter_model->mapFromSource(index);
		m_scenes->selectionModel()->blockSignals(true);
		m_scenes->clearSelection();
		m_scenes->setCurrentIndex(index);
		m_scenes->scrollTo(index);
		m_scenes->selectionModel()->blockSignals(false);
	}
}

//-----------------------------------------------------------------------------

void SceneList::setFilter(const QString& filter)
{
	m_filter_model->setFilterFixedString(filter);
	if (filter.isEmpty()) {
		m_scenes->setDragEnabled(true);
		m_scenes->setSelectionMode(QAbstractItemView::ExtendedSelection);
	} else {
		m_scenes->setDragEnabled(false);
		m_scenes->setSelectionMode(QAbstractItemView::SingleSelection);
	}
}

//-----------------------------------------------------------------------------

void SceneList::toggleScenes()
{
	if (scenesVisible()) {
		hideScenes();
	} else {
		showScenes();
	}
}

//-----------------------------------------------------------------------------

void SceneList::updateShortcuts()
{
	QKeySequence shortcut = ActionManager::instance()->action("ToggleScenes")->shortcut();
	m_toggle_action->setShortcut(shortcut);
	m_show_button->setToolTip(tr("Show scene list (%1)").arg(shortcut.toString(QKeySequence::NativeText)));
	m_hide_button->setToolTip(tr("Hide scene list (%1)").arg(shortcut.toString(QKeySequence::NativeText)));
}

//-----------------------------------------------------------------------------

void SceneList::moveSelectedScenes(int movement)
{
	// Find scenes to move
	QModelIndexList indexes = m_filter_model->mapSelectionToSource(m_scenes->selectionModel()->selection()).indexes();
	if (indexes.isEmpty()) {
		return;
	}
	QList<int> scenes;

	// Find target row
	int first_row = INT_MAX;
	int last_row = 0;
	int index_row = 0;
	for (int i = 0, count = indexes.count(); i < count; ++i) {
		index_row = indexes.at(i).row();
		first_row = std::min(first_row, index_row);
		last_row = std::max(last_row, index_row);
		scenes.append(index_row);
	}
	int row = std::max(0, ((movement > 0) ? (last_row + 1) : first_row) + movement);

	// Move scenes
	m_document->sceneModel()->moveScenes(scenes, row);
}

//-----------------------------------------------------------------------------
