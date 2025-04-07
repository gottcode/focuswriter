/*
	SPDX-FileCopyrightText: 2012-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "symbols_dialog.h"

#include "action_manager.h"
#include "shortcut_edit.h"
#include "symbols_model.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTableView>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

//-----------------------------------------------------------------------------

class SymbolsDialog::ElideLabel : public QFrame
{
public:
	explicit ElideLabel(QWidget* parent = nullptr);

	void clear();
	void setText(const QString& text);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	QString m_text;
};

SymbolsDialog::ElideLabel::ElideLabel(QWidget* parent)
	: QFrame(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void SymbolsDialog::ElideLabel::clear()
{
	m_text.clear();
	update();
}

void SymbolsDialog::ElideLabel::setText(const QString& text)
{
	m_text = text;
	update();
}

void SymbolsDialog::ElideLabel::paintEvent(QPaintEvent* event)
{
	QFrame::paintEvent(event);

	QPainter painter(this);
	const QFontMetrics metrics = painter.fontMetrics();

	const QString text = metrics.elidedText(m_text, Qt::ElideRight, width());
	painter.drawText(QPointF(0, metrics.ascent()), text);
}

//-----------------------------------------------------------------------------

SymbolsDialog::SymbolsDialog(QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(tr("Symbols"));

	m_model = new SymbolsModel(this);

	m_contents = new QSplitter(this);

	// Create table to show recently used symbols
	QGroupBox* recent_group = new QGroupBox(tr("Recently used symbols"), this);

	m_recent = new QTableWidget(recent_group);
	m_recent->setColumnCount(16);
	m_recent->setRowCount(1);
	m_recent->setSelectionMode(QAbstractItemView::SingleSelection);
	m_recent->setTabKeyNavigation(false);
	m_recent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_recent->verticalHeader()->setSectionsClickable(false);
	m_recent->verticalHeader()->setSectionsMovable(false);
	m_recent->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_recent->setMaximumHeight(m_recent->verticalHeader()->sectionSize(0));
	m_recent->horizontalHeader()->hide();
	m_recent->verticalHeader()->hide();
	m_recent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_recent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	connect(m_recent, &QTableWidget::currentItemChanged, this, &SymbolsDialog::recentSymbolClicked);
	connect(m_recent, &QTableWidget::doubleClicked, this, &SymbolsDialog::accept);

	QVBoxLayout* recent_layout = new QVBoxLayout(recent_group);
	recent_layout->addWidget(m_recent);

	// Create filter list widgets
	QGroupBox* symbols_group = new QGroupBox(tr("All symbols"), this);

	QWidget* sidebar = new QWidget(symbols_group);
	m_contents->addWidget(sidebar);

	m_groups = new QComboBox(sidebar);
	connect(m_groups, &QComboBox::activated, this, &SymbolsDialog::showGroup);

	QVBoxLayout* sidebar_layout = new QVBoxLayout(sidebar);
	sidebar_layout->setContentsMargins(0, 0, 0, 0);
	sidebar_layout->addWidget(m_groups);

	const QStringList groups = m_model->filterGroups();
	for (int i = 0, count = groups.count(); i < count; ++i) {
		m_groups->addItem(groups.at(i));

		QListWidget* filters = new QListWidget(sidebar);
		sidebar_layout->addWidget(filters, 1);
		m_filters += filters;

		const QStringList names = m_model->filters(i);
		for (int j = 0, j_count = names.count(); j < j_count; ++j) {
			QListWidgetItem* item = new QListWidgetItem(names.at(j), filters);
			item->setData(Qt::UserRole, j);
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		}
		filters->sortItems();
	}

	// Create table to show all symbols
	m_view = new QTableView(symbols_group);
	m_view->setSelectionMode(QAbstractItemView::SingleSelection);
	m_view->setTabKeyNavigation(false);
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	m_view->horizontalHeader()->setSectionsClickable(false);
	m_view->horizontalHeader()->setSectionsMovable(false);
	m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_view->verticalHeader()->setSectionsClickable(false);
	m_view->verticalHeader()->setSectionsMovable(false);
	m_view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	m_view->horizontalHeader()->hide();
	m_view->verticalHeader()->hide();
	m_view->setModel(m_model);
	connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &SymbolsDialog::symbolClicked);
	connect(m_view, &QTableView::doubleClicked, this, &SymbolsDialog::accept);

	m_contents->addWidget(m_view);
	m_contents->setStretchFactor(1, 1);

	QVBoxLayout* symbols_layout = new QVBoxLayout(symbols_group);
	symbols_layout->addWidget(m_contents);

	// Create details widgets
	QGroupBox* details_group = new QGroupBox(tr("Details"), this);

	QGraphicsScene* scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(palette().base());
	m_symbol_preview_item = scene->addSimpleText(QString());
	m_symbol_preview_item->setBrush(palette().text());
	m_symbol_preview = new QGraphicsView(scene, details_group);
	m_symbol_preview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_symbol_preview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	const int size = fontMetrics().height() * 4;
	m_symbol_preview->setFixedSize(size, size);

	m_symbol_shortcut = new ShortcutEdit(details_group);
	connect(m_symbol_shortcut, &ShortcutEdit::changed, this, &SymbolsDialog::shortcutChanged);

	m_symbol_name = new ElideLabel(details_group);

	m_symbol_code = new QLabel(details_group);

	QGridLayout* details_layout = new QGridLayout(details_group);
	details_layout->setColumnStretch(2, 1);
	details_layout->setRowStretch(0, 1);
	details_layout->setRowStretch(3, 1);
	details_layout->addWidget(m_symbol_preview, 0, 0, 4, 1, Qt::AlignCenter);
	details_layout->addWidget(new QLabel(ShortcutEdit::tr("Shortcut:"), details_group), 1, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
	details_layout->addWidget(m_symbol_shortcut, 1, 2, 1, 3);
	details_layout->addWidget(new QLabel(tr("Name:"), details_group), 2, 1, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
	details_layout->addWidget(m_symbol_name, 2, 2);
	details_layout->addWidget(m_symbol_code, 2, 3);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	buttons->button(QDialogButtonBox::Close)->setAutoDefault(false);
	connect(buttons, &QDialogButtonBox::accepted, this, &SymbolsDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &SymbolsDialog::reject);

	m_insert_button = buttons->addButton(tr("Insert"), QDialogButtonBox::AcceptRole);
	m_insert_button->setAutoDefault(true);
	m_insert_button->setDefault(true);

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(recent_group);
	layout->addWidget(symbols_group, 1);
	layout->addWidget(details_group);

	layout->addWidget(buttons);

	// Restore size of dialog
	QSettings settings;
	resize(settings.value("SymbolsDialog/Size", QSize(750,500)).toSize());
	m_contents->setSizes({ 200, 550 });
	m_contents->restoreState(settings.value("SymbolsDialog/SplitterSizes").toByteArray());

	// Switch to last used tab
	m_groups->setCurrentIndex(settings.value("SymbolsDialog/Group", 1).toInt());
	showGroup(m_groups->currentIndex());

	// Fetch list of recently used symbols
	const QVariantList recent = settings.value("SymbolsDialog/Recent").toList();
	for (int i = 0, count = std::min(16, int(recent.count())); i < count; ++i) {
		const char32_t unicode = recent.at(i).toUInt();
		QTableWidgetItem* item = new QTableWidgetItem(QString::fromUcs4(&unicode, 1));
		item->setTextAlignment(Qt::AlignCenter);
		item->setData(Qt::UserRole, unicode);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		m_recent->setItem(0, i, item);
	}
	for (int i = recent.count(); i < 16; ++i) {
		QTableWidgetItem* item = new QTableWidgetItem;
		item->setBackground(palette().button());
		item->setFlags(Qt::NoItemFlags);
		m_recent->setItem(0, i, item);
	}

	// Select most recently selected symbol
	selectSymbol(settings.value("SymbolsDialog/Current", ' ').toUInt());
}

//-----------------------------------------------------------------------------

void SymbolsDialog::setInsertEnabled(bool enabled)
{
	if (m_insert_button->isEnabled() == enabled) {
		return;
	}

	m_insert_button->setEnabled(enabled);

	if (enabled) {
		connect(m_view, &QTableView::doubleClicked, this, &SymbolsDialog::accept);
	} else {
		disconnect(m_view, &QTableView::doubleClicked, this, &SymbolsDialog::accept);
	}
}

//-----------------------------------------------------------------------------

void SymbolsDialog::setPreviewFont(const QFont& font)
{
	const QFontMetrics metrics(font);
	m_symbol_preview_item->setFont(font);
	m_symbol_preview->fitInView(0, 0, metrics.height() * 1.5, metrics.height() * 1.5, Qt::KeepAspectRatio);
}

//-----------------------------------------------------------------------------

void SymbolsDialog::accept()
{
	const QModelIndex symbol = m_view->currentIndex();
	if (symbol.isValid()) {
		const char32_t unicode = symbol.internalId();

		// Remove symbol from recent list
		for (int i = 0, count = m_recent->columnCount(); i < count; ++i) {
			const QTableWidgetItem* item = m_recent->item(0, i);
			if (item && (item->data(Qt::UserRole).toUInt() == unicode)) {
				m_recent->removeColumn(i);
				break;
			}
		}

		// Prepend symbol to recent list
		m_recent->insertColumn(0);
		while (m_recent->columnCount() > 16) {
			m_recent->removeColumn(15);
		}
		QTableWidgetItem* item = new QTableWidgetItem(symbol.data(Qt::DisplayRole).toString());
		item->setTextAlignment(Qt::AlignCenter);
		item->setData(Qt::UserRole, unicode);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		m_recent->setItem(0, 0, item);
		m_recent->clearSelection();
		item->setSelected(true);

		Q_EMIT insertText(QString::fromUcs4(&unicode, 1));
	}

	saveSettings();
	QDialog::accept();
}

//-----------------------------------------------------------------------------

void SymbolsDialog::reject()
{
	saveSettings();
	QDialog::reject();
}

//-----------------------------------------------------------------------------

void SymbolsDialog::showEvent(QShowEvent* event)
{
	const QFontMetrics metrics(m_symbol_preview_item->font());
	const float size = metrics.height() * 1.5f;
	m_symbol_preview->fitInView(0, 0, size, size);

	m_view->setFocus();
	QDialog::showEvent(event);
}

//-----------------------------------------------------------------------------

void SymbolsDialog::showFilter(const QListWidgetItem* filter)
{
	if (!filter) {
		return;
	}
	m_model->setFilter(m_groups->currentIndex(), filter->data(Qt::UserRole).toInt());
	m_view->setCurrentIndex(m_model->index(0, 0));
}

//-----------------------------------------------------------------------------

void SymbolsDialog::showGroup(int group)
{
	for (QListWidget* filters : std::as_const(m_filters)) {
		disconnect(filters, &QListWidget::currentItemChanged, this, &SymbolsDialog::showFilter);
		filters->clearSelection();
		filters->hide();
	}

	QListWidget* filters = m_filters.at(group);
	filters->show();

	if (m_model->rowCount()) {
		const QModelIndex symbol = m_view->currentIndex();
		const char32_t unicode = symbol.internalId();

		if (!selectSymbol(unicode)) {
			selectSymbol(' ');
		}
	} else {
		filters->clearSelection();
	}

	connect(filters, &QListWidget::currentItemChanged, this, &SymbolsDialog::showFilter);
}

//-----------------------------------------------------------------------------

void SymbolsDialog::symbolClicked(const QModelIndex& symbol)
{
	if (symbol.isValid()) {
		// Show symbol details
		const char32_t unicode = symbol.internalId();
		const QString name = m_model->symbolName(unicode);
		m_symbol_preview_item->setText(symbol.data(Qt::DisplayRole).toString());
		m_symbol_preview->setSceneRect(m_symbol_preview_item->boundingRect());
		m_symbol_name->setText(name);
		m_symbol_name->setToolTip(name);
		m_symbol_code->setText(QString("<tt>U+%1</tt>").arg(static_cast<quint32>(unicode), 4, 16, QLatin1Char('0')).toUpper());
		m_symbol_shortcut->setShortcut(ActionManager::instance()->shortcut(unicode));

		// Select symbol in recent list, and clear any other selections
		for (int i = 0, count = m_recent->columnCount(); i < count; ++i) {
			QTableWidgetItem* item = m_recent->item(0, i);
			if (item) {
				item->setSelected(item->data(Qt::UserRole).toUInt() == unicode);
			}
		}
	} else {
		m_symbol_name->clear();
		m_symbol_code->clear();
	}
}

//-----------------------------------------------------------------------------

void SymbolsDialog::recentSymbolClicked(const QTableWidgetItem* symbol)
{
	if (!symbol) {
		return;
	}

	selectSymbol(symbol->data(Qt::UserRole).toUInt());
}

//-----------------------------------------------------------------------------

void SymbolsDialog::shortcutChanged()
{
	const char32_t unicode = m_view->currentIndex().internalId();
	const QKeySequence sequence = m_symbol_shortcut->shortcut();
	ActionManager::instance()->setShortcut(unicode, sequence);
}

//-----------------------------------------------------------------------------

bool SymbolsDialog::selectSymbol(char32_t unicode)
{
	const int group = m_groups->currentIndex();

	// Select filter for symbol
	QListWidget* filters = m_filters.at(group);
	const int filter = m_model->symbolFilter(group, unicode);
	if (filter == -1) {
		return false;
	}
	QListWidgetItem* item = filters->currentItem();
	if (!item || !item->isSelected() || (item->data(Qt::UserRole).toInt() != filter)) {
		item = nullptr;
		for (int i = 0, count = filters->count(); i < count; ++i) {
			QListWidgetItem* check = filters->item(i);
			if (check->data(Qt::UserRole).toInt() == filter) {
				item = check;
				break;
			}
		}
		if (!item) {
			return false;
		}

		filters->blockSignals(true);
		filters->setCurrentItem(item);
		filters->scrollToItem(item);
		m_model->setFilter(group, filter);
		filters->blockSignals(false);
	}

	// Select symbol in table
	const QModelIndex symbol = m_model->index(unicode);
	m_view->setCurrentIndex(symbol);
	m_view->scrollTo(symbol);

	return true;
}

//-----------------------------------------------------------------------------

void SymbolsDialog::saveSettings()
{
	QSettings settings;
	settings.setValue("SymbolsDialog/Size", size());
	settings.setValue("SymbolsDialog/SplitterSizes", m_contents->saveState());
	settings.setValue("SymbolsDialog/Group", m_groups->currentIndex());
	QVariantList recent;
	for (int i = 0; i < 16; ++i) {
		const QTableWidgetItem* item = m_recent->item(0, i);
		if (item && (item->flags() & Qt::ItemIsEnabled)) {
			recent += item->data(Qt::UserRole);
		}
	}
	settings.setValue("SymbolsDialog/Recent", recent);
	settings.setValue("SymbolsDialog/Current", m_view->currentIndex().internalId());
}

//-----------------------------------------------------------------------------
