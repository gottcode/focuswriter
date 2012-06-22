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

#include "scene_list.h"

#include "document.h"
#include "scene_model.h"

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTextBlock>
#include <QTextEdit>
#include <QToolButton>

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
	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);
	const QWidget* widget = opt.widget;
	const QStyle* style = widget ? widget->style() : QApplication::style();

	QSize size = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);
	int margin = style->pixelMetric(QStyle::PM_FocusFrameVMargin, &opt, widget) * 2;
	int height = opt.fontMetrics.lineSpacing() * 3;
	size.setHeight(margin + height);
	return size;
}

}

//-----------------------------------------------------------------------------

SceneList::SceneList(QWidget* parent) :
	QFrame(parent),
	m_document(0)
{
	setMaximumWidth(336);

	// Configure sidebar
	setFrameStyle(QFrame::Panel | QFrame::Raised);
	setAutoFillBackground(true);
	setPalette(QApplication::palette());

	// Create button to show scenes
	m_show_button = new QToolButton(this);
	m_show_button->setShortcut(tr("Shift+F4"));
	m_show_button->setToolTip(tr("Show scene list (%1)").arg(m_show_button->shortcut().toString(QKeySequence::NativeText)));
	m_show_button->setAutoRaise(true);
	m_show_button->setArrowType(Qt::RightArrow);
	m_show_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
	connect(m_show_button, SIGNAL(clicked()), this, SLOT(showScenes()));

	// Create button to hide scenes
	m_hide_button = new QToolButton(this);
	m_hide_button->setShortcut(tr("Shift+F4"));
	m_hide_button->setToolTip(tr("Hide scene list (%1)").arg(m_hide_button->shortcut().toString(QKeySequence::NativeText)));
	m_hide_button->setAutoRaise(true);
	m_hide_button->setArrowType(Qt::LeftArrow);
	m_hide_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
	connect(m_hide_button, SIGNAL(clicked()), this, SLOT(hideScenes()));

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
	connect(m_scenes->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(sceneSelected(QModelIndex)));
	m_scenes->show();

	// Create filter widget
	m_filter = new QLineEdit(this);
#if (QT_VERSION >= QT_VERSION_CHECK(4,7,0))
	m_filter->setPlaceholderText(tr("Filter"));
#endif
	connect(m_filter, SIGNAL(textChanged(QString)), this, SLOT(setFilter(QString)));

	// Lay out widgets
	QGridLayout* layout = new QGridLayout(this);
	layout->setMargin(layout->spacing());
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(0, 1);
	layout->addWidget(m_show_button, 0, 0, 2, 1);
	layout->addWidget(m_hide_button, 0, 1, 2, 1);
	layout->addWidget(m_scenes, 0, 2);
	layout->addWidget(m_filter, 1, 2);

	// Start collapsed
	hideScenes();
}

//-----------------------------------------------------------------------------

bool SceneList::scenesVisible() const
{
	return m_scenes->isVisible();
}

//-----------------------------------------------------------------------------

void SceneList::setDocument(Document* document)
{
	m_document = 0;

	m_scenes->clearSelection();
	m_filter->clear();
	m_filter_model->setSourceModel(document->sceneModel());

	QModelIndex index = document->sceneModel()->findScene(document->text()->textCursor());
	if (index.isValid()) {
		index = m_filter_model->mapFromSource(index);
		m_scenes->setCurrentIndex(index);
	}

	m_document = document;
}

//-----------------------------------------------------------------------------

void SceneList::resizeEvent(QResizeEvent* event)
{
	m_scenes->scrollTo(m_scenes->currentIndex());
	QFrame::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void SceneList::hideScenes()
{
	m_show_button->show();

	m_hide_button->hide();
	m_scenes->hide();
	m_filter->hide();

	setMaximumWidth(minimumSizeHint().width());

	m_filter->clear();

	if (m_document) {
		m_document->text()->setFocus();
	}

	if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
		setMask(QRect(-1,-1,1,1));
	}
}

//-----------------------------------------------------------------------------

void SceneList::showScenes()
{
	clearMask();

	m_hide_button->show();
	m_scenes->show();
	m_filter->show();

	m_show_button->hide();

	setMaximumWidth(336);

	m_scenes->setFocus();
}

//-----------------------------------------------------------------------------

void SceneList::sceneSelected(const QModelIndex& index)
{
	if (!m_document) {
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
