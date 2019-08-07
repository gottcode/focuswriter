/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "stack.h"

#include "action_manager.h"
#include "alert.h"
#include "alert_layer.h"
#include "document.h"
#include "find_dialog.h"
#include "preferences.h"
#include "scene_list.h"
#include "scene_model.h"
#include "smart_quotes.h"
#include "symbols_dialog.h"
#include "theme_renderer.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QGridLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPaintEvent>
#include <QPrinter>
#include <QStackedWidget>
#include <QStyle>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>

#include <algorithm>

//-----------------------------------------------------------------------------

Stack::Stack(QWidget* parent) :
	QWidget(parent),
	m_symbols_dialog(0),
	m_printer(0),
	m_current_document(0),
	m_footer_margin(0),
	m_header_margin(0),
	m_footer_visible(0),
	m_header_visible(0)
{
	setMouseTracking(true);

	m_contents = new QStackedWidget(this);

	m_alerts = new AlertLayer(this);

	m_scenes = new SceneList(this);
	setScenesVisible(false);

	m_menu = new QMenu(this);
	m_menu_group = new QActionGroup(this);
	m_menu_group->setExclusive(true);
	connect(m_menu_group, &QActionGroup::triggered, this, &Stack::actionTriggered);

	m_find_dialog = new FindDialog(this);
	connect(m_find_dialog, &FindDialog::findNextAvailable, this, &Stack::findNextAvailable);

	connect(ActionManager::instance(), &ActionManager::insertText, this, &Stack::insertSymbol);

	m_layout = new QGridLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSpacing(0);
	m_layout->setRowMinimumHeight(1, 6);
	m_layout->setRowMinimumHeight(4, 6);
	m_layout->setRowStretch(2, 1);
	m_layout->setColumnMinimumWidth(1, 6);
	m_layout->setColumnMinimumWidth(4, 6);
	m_layout->setColumnStretch(1, 1);
	m_layout->setColumnStretch(2, 1);
	m_layout->setColumnStretch(3, 1);
	m_layout->addWidget(m_contents, 1, 0, 4, 6);
	m_layout->addWidget(m_scenes, 1, 0, 4, 3);
	m_layout->addWidget(m_alerts, 3, 3);

	m_resize_timer = new QTimer(this);
	m_resize_timer->setInterval(50);
	m_resize_timer->setSingleShot(true);
	connect(m_resize_timer, &QTimer::timeout, this, QOverload<>::of(&Stack::updateBackground));

	m_theme_renderer = new ThemeRenderer(this);
	connect(m_theme_renderer, &ThemeRenderer::rendered, this, QOverload<const QImage&, const QRect&>::of(&Stack::updateBackground));

	setHeaderVisible(Preferences::instance().alwaysShowHeader());
	setFooterVisible(Preferences::instance().alwaysShowFooter());

	// Always draw background
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	updateBackground();
}

//-----------------------------------------------------------------------------

Stack::~Stack()
{
	m_theme_renderer->wait();

	delete m_printer;
}

//-----------------------------------------------------------------------------

void Stack::addDocument(Document* document)
{
	document->setSceneList(m_scenes);
	connect(document, &Document::alert, m_alerts, &AlertLayer::addAlert);
	connect(document, &Document::alignmentChanged, this, &Stack::updateFormatAlignmentActions);
	connect(document, &Document::changedName, this, &Stack::updateFormatActions);
	connect(document, &Document::footerVisible, this, &Stack::setFooterVisible);
	connect(document, &Document::headerVisible, this, &Stack::setHeaderVisible);
	connect(document, &Document::scenesVisible, this, &Stack::setScenesVisible);
	connect(document->text(), &QTextEdit::copyAvailable, this, &Stack::copyAvailable);
	connect(document->text(), &QTextEdit::redoAvailable, this, &Stack::redoAvailable);
	connect(document->text(), &QTextEdit::undoAvailable, this, &Stack::undoAvailable);
	connect(document->text(), &QTextEdit::currentCharFormatChanged, this, &Stack::updateFormatActions);

	m_documents.append(document);
	m_contents->addWidget(document);
	m_contents->setCurrentWidget(document);

	QAction* action = new QAction(this);
	action->setCheckable(true);
	action->setActionGroup(m_menu_group);
	m_document_actions.push_back(action);
	m_menu->addAction(action);
	updateMenuIndexes();

	document->loadTheme(m_theme);
	document->text()->setFixedSize(m_foreground_size);

	emit documentAdded(document);
	emit updateFormatActions();
}

//-----------------------------------------------------------------------------

void Stack::moveDocument(int from, int to)
{
	QAction* action = m_document_actions.at(from);
	QAction* before = m_document_actions.at(to);
	m_menu->removeAction(action);
	m_documents.move(from, to);
	m_document_actions.move(from, to);
	m_menu->insertAction(before, action);
	updateMenuIndexes();
}

//-----------------------------------------------------------------------------

void Stack::removeDocument(int index)
{
	Document* document = m_documents.takeAt(index);
	m_contents->removeWidget(document);

	QAction* action = m_document_actions.takeAt(index);
	m_menu->removeAction(action);
	delete action;
	updateMenuIndexes();

	emit documentRemoved(document);
	document->close();
}

//-----------------------------------------------------------------------------

void Stack::updateDocument(int index)
{
	Document* document = m_documents.at(index);
	QAction* action = m_document_actions.at(index);
	action->setText(document->title() + (document->isModified() ? "*" : ""));
	action->setToolTip(QDir::toNativeSeparators(document->filename()));
}

//-----------------------------------------------------------------------------

void Stack::setCurrentDocument(int index)
{
	m_current_document = m_documents[index];
	m_contents->setCurrentWidget(m_current_document);
	m_scenes->setDocument(m_current_document);
	m_document_actions[index]->setChecked(true);

	emit copyAvailable(!m_current_document->text()->textCursor().selectedText().isEmpty());
	emit redoAvailable(m_current_document->text()->document()->isRedoAvailable());
	emit undoAvailable(m_current_document->text()->document()->isUndoAvailable());
	emit updateFormatActions();
}

//-----------------------------------------------------------------------------

void Stack::setMargins(int footer, int header)
{
	m_footer_margin = footer;
	m_header_margin = header;
	m_footer_visible = (m_footer_visible != 0) ? -m_footer_margin : 0;
	m_header_visible = (m_header_visible != 0) ? m_header_margin : 0;
	updateMargin();
	updateBackground();
	updateMask();
	showHeader();
}

//-----------------------------------------------------------------------------

void Stack::waitForThemeBackground()
{
	if (m_theme_renderer->isRunning()) {
		m_theme_renderer->wait();
		repaint();
	}
}

//-----------------------------------------------------------------------------

bool Stack::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::MouseMove) {
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
	}
	return QWidget::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------

void Stack::alignCenter()
{
	m_current_document->setRichText(true);
	m_current_document->text()->setAlignment(Qt::AlignCenter);
}

//-----------------------------------------------------------------------------

void Stack::alignJustify()
{
	m_current_document->setRichText(true);
	m_current_document->text()->setAlignment(Qt::AlignJustify);
}

//-----------------------------------------------------------------------------

void Stack::alignLeft()
{
	m_current_document->setRichText(true);
	m_current_document->text()->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
}

//-----------------------------------------------------------------------------

void Stack::alignRight()
{
	m_current_document->setRichText(true);
	m_current_document->text()->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
}

//-----------------------------------------------------------------------------

void Stack::autoCache()
{
	for (Document* document : m_documents) {
		if (document->isModified()) {
			document->cache();
		}
	}
}

//-----------------------------------------------------------------------------

void Stack::checkSpelling()
{
	m_current_document->checkSpelling();
}

//-----------------------------------------------------------------------------

void Stack::cut()
{
	m_current_document->text()->cut();
}

//-----------------------------------------------------------------------------

void Stack::copy()
{
	m_current_document->text()->copy();
}

//-----------------------------------------------------------------------------

void Stack::decreaseIndent()
{
	m_current_document->setRichText(true);
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat format = cursor.blockFormat();
	format.setIndent(std::max(0, format.indent() - 1));
	cursor.setBlockFormat(format);
	emit updateFormatActions();
}

//-----------------------------------------------------------------------------

void Stack::find()
{
	m_find_dialog->showFindMode();
}

//-----------------------------------------------------------------------------

void Stack::findNext()
{
	m_find_dialog->findNext();
}

//-----------------------------------------------------------------------------

void Stack::findPrevious()
{
	m_find_dialog->findPrevious();
}

//-----------------------------------------------------------------------------

void Stack::increaseIndent()
{
	m_current_document->setRichText(true);
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat format = cursor.blockFormat();
	format.setIndent(format.indent() + 1);
	cursor.setBlockFormat(format);
	emit updateFormatActions();
}

//-----------------------------------------------------------------------------

void Stack::paste()
{
	m_current_document->text()->paste();
}

//-----------------------------------------------------------------------------

void Stack::pasteUnformatted()
{
	QString text = QApplication::clipboard()->text(QClipboard::Clipboard);
	m_current_document->text()->insertPlainText(text);
}

//-----------------------------------------------------------------------------

void Stack::pageSetup()
{
	initPrinter();
	QPageSetupDialog dialog(m_printer, this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Stack::print()
{
	initPrinter();
	m_current_document->print(m_printer);
}

//-----------------------------------------------------------------------------

void Stack::redo()
{
	m_current_document->text()->redo();
}

//-----------------------------------------------------------------------------

void Stack::reload()
{
	m_current_document->reload();
}

//-----------------------------------------------------------------------------

void Stack::replace()
{
	m_find_dialog->showReplaceMode();
}

//-----------------------------------------------------------------------------

void Stack::save()
{
	m_current_document->save();
}

//-----------------------------------------------------------------------------

void Stack::saveAs()
{
	m_current_document->saveAs();
}

//-----------------------------------------------------------------------------

void Stack::selectAll()
{
	m_current_document->text()->selectAll();
}

//-----------------------------------------------------------------------------

void Stack::selectScene()
{
	m_current_document->sceneModel()->selectScene();
}

//-----------------------------------------------------------------------------

void Stack::setFocusMode(QAction* action)
{
	int focus_mode = action->data().toInt();
	for (Document* document : m_documents) {
		document->setFocusMode(focus_mode);
	}
}

//-----------------------------------------------------------------------------

void Stack::setBlockHeading(int heading)
{
	m_current_document->setRichText(true);
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat block_format = cursor.blockFormat();
	block_format.setProperty(QTextFormat::UserProperty, heading);
	cursor.setBlockFormat(block_format);
}

//-----------------------------------------------------------------------------

void Stack::setFontBold(bool bold)
{
	m_current_document->setRichText(true);
	m_current_document->text()->setFontWeight(bold ? QFont::Bold : QFont::Normal);
}

//-----------------------------------------------------------------------------

void Stack::setFontItalic(bool italic)
{
	m_current_document->setRichText(true);
	m_current_document->text()->setFontItalic(italic);
}

//-----------------------------------------------------------------------------

void Stack::setFontStrikeOut(bool strikeout)
{
	m_current_document->setRichText(true);
	QTextCharFormat format;
	format.setFontStrikeOut(strikeout);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

//-----------------------------------------------------------------------------

void Stack::setFontUnderline(bool underline)
{
	m_current_document->setRichText(true);
	m_current_document->text()->setFontUnderline(underline);
}

//-----------------------------------------------------------------------------

void Stack::setFontSuperScript(bool super)
{
	m_current_document->setRichText(true);
	QTextCharFormat format;
	format.setVerticalAlignment(super ? QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

//-----------------------------------------------------------------------------

void Stack::setFontSubScript(bool sub)
{
	m_current_document->setRichText(true);
	QTextCharFormat format;
	format.setVerticalAlignment(sub ? QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

//-----------------------------------------------------------------------------

void Stack::setTextDirectionLTR()
{
	if (m_current_document) {
		m_current_document->setRichText(true);
		QTextCursor cursor = m_current_document->text()->textCursor();
		QTextBlockFormat format = cursor.blockFormat();
		format.setLayoutDirection(Qt::LeftToRight);
		format.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
		cursor.mergeBlockFormat(format);
		emit updateFormatAlignmentActions();
	}
}

//-----------------------------------------------------------------------------

void Stack::setTextDirectionRTL()
{
	if (m_current_document) {
		m_current_document->setRichText(true);
		QTextCursor cursor = m_current_document->text()->textCursor();
		QTextBlockFormat format = cursor.blockFormat();
		format.setLayoutDirection(Qt::RightToLeft);
		format.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
		cursor.mergeBlockFormat(format);
		emit updateFormatAlignmentActions();
	}
}

//-----------------------------------------------------------------------------

void Stack::showSymbols()
{
	// Load symbols dialog on demand
	if (!m_symbols_dialog) {
		window()->setCursor(Qt::WaitCursor);
		m_symbols_dialog = new SymbolsDialog(this);
		m_symbols_dialog->setInsertEnabled(!m_current_document->isReadOnly());
		m_symbols_dialog->setPreviewFont(m_current_document->text()->font());
		connect(m_symbols_dialog, &SymbolsDialog::insertText, this, &Stack::insertSymbol);
		window()->unsetCursor();
	}

	// Show dialog
	m_symbols_dialog->show();
	m_symbols_dialog->raise();
	m_symbols_dialog->activateWindow();
}

//-----------------------------------------------------------------------------

void Stack::themeSelected(const Theme& theme)
{
	m_theme = theme;

	if (m_symbols_dialog) {
		m_symbols_dialog->setPreviewFont(m_theme.textFont());
	}

	updateMargin();
	updateBackground();

	for (Document* document : m_documents) {
		document->loadTheme(theme);
	}
}

//-----------------------------------------------------------------------------

void Stack::undo()
{
	m_current_document->text()->undo();
}

//-----------------------------------------------------------------------------

void Stack::updateSmartQuotes()
{
	SmartQuotes::replace(m_current_document->text(), 0, m_current_document->text()->document()->characterCount());
	m_current_document->centerCursor(true);
}

//-----------------------------------------------------------------------------

void Stack::updateSmartQuotesSelection()
{
	QTextCursor cursor = m_current_document->text()->textCursor();
	SmartQuotes::replace(m_current_document->text(), cursor.selectionStart(), cursor.selectionEnd());
}

//-----------------------------------------------------------------------------

void Stack::setFooterVisible(bool visible)
{
	visible |= Preferences::instance().alwaysShowFooter();
	int footer_visible = visible * -m_footer_margin;
	if (m_footer_visible != footer_visible) {
		emit footerVisible(visible);
		m_footer_visible = footer_visible;
		updateMask();
	}
}

//-----------------------------------------------------------------------------

void Stack::setHeaderVisible(bool visible)
{
	visible |= Preferences::instance().alwaysShowHeader();
	int header_visible = visible * m_header_margin;
	if (m_header_visible != header_visible) {
		emit headerVisible(visible);
		m_header_visible = header_visible;
		updateMask();
	}
}

//-----------------------------------------------------------------------------

void Stack::setScenesVisible(bool visible)
{
	if (!visible && !m_scenes->scenesVisible()) {
		m_scenes->hide();
	} else {
		m_scenes->show();
	}
}

//-----------------------------------------------------------------------------

void Stack::showHeader()
{
	QPoint point = mapFromGlobal(QCursor::pos());
	setHeaderVisible(window()->rect().contains(point) && point.y() <= m_header_margin);
}

//-----------------------------------------------------------------------------

void Stack::mouseMoveEvent(QMouseEvent* event)
{
	int y = mapFromGlobal(event->globalPos()).y();
	bool header_visible = y <= m_header_margin;
	bool footer_visible = y >= (height() - m_footer_margin);
	setHeaderVisible(header_visible);
	setFooterVisible(footer_visible);
	setScenesVisible(false);

	if (m_current_document) {
		if (header_visible || footer_visible) {
			m_current_document->setScrollBarVisible(false);
		} else {
			m_current_document->mouseMoveEvent(event);
		}
	}
}

//-----------------------------------------------------------------------------

void Stack::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	const qreal pixelratio = devicePixelRatioF();
	const QRectF rect(event->rect().topLeft() * pixelratio, event->rect().size() * pixelratio);
	painter.drawPixmap(event->rect(), m_background, rect);
	painter.end();
}

//-----------------------------------------------------------------------------

void Stack::resizeEvent(QResizeEvent* event)
{
	updateMask();
	m_resize_timer->start();
	updateBackground();
	QWidget::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void Stack::actionTriggered(QAction* action)
{
	emit documentSelected(action->data().toInt());
}

//-----------------------------------------------------------------------------

void Stack::insertSymbol(const QString& text)
{
	m_current_document->text()->insertPlainText(text);
}

//-----------------------------------------------------------------------------

void Stack::updateBackground()
{
	const int margin = m_layout->rowMinimumHeight(0);
	const qreal pixelratio = devicePixelRatioF();

	// Create temporary background
	const QRectF foreground = m_theme.foregroundRect(size(), margin, pixelratio);

	QImage image(size() * pixelratio, QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(pixelratio);
	image.fill(m_theme.loadColor().rgb());
	{
		QPainter painter(&image);
		QColor color = m_theme.foregroundColor();
		color.setAlpha(m_theme.foregroundOpacity() * 2.55f);
		painter.setPen(Qt::NoPen);
		painter.setBrush(color);

		if (!m_theme.roundCornersEnabled()) {
			painter.drawRect(foreground);
		} else {
			painter.setRenderHint(QPainter::Antialiasing);
			painter.drawRoundedRect(foreground, m_theme.cornerRadius(), m_theme.cornerRadius());
		}
	}

	updateBackground(image, foreground.toRect());

	// Create proper background
	if (!m_resize_timer->isActive()) {
		m_theme_renderer->create(m_theme, size(), margin, pixelratio);
	}
}

//-----------------------------------------------------------------------------

void Stack::updateBackground(const QImage& image, const QRect& foreground)
{
	// Make sure image is correct size
	if (image.size() != (size() * devicePixelRatioF())) {
		return;
	}

	// Load background
	m_background = QPixmap::fromImage(image, Qt::AutoColor | Qt::AvoidDither);
	m_background.setDevicePixelRatio(devicePixelRatioF());

	// Determine text area size
	const int padding = m_theme.foregroundPadding();
	const QRect foreground_rect = foreground.adjusted(padding, padding, -padding, -padding);
	if (!m_resize_timer->isActive() && (foreground_rect.size() != m_foreground_size)) {
		m_foreground_size = foreground_rect.size();

		for (Document* document : m_documents) {
			document->text()->setFixedSize(m_foreground_size);
			document->centerCursor(true);
		}
	}

	update();
}

//-----------------------------------------------------------------------------

void Stack::updateMargin()
{
	int margin = std::max(m_theme.foregroundMargin().value(), 1);
	if (Preferences::instance().alwaysShowFooter()) {
		margin = std::max(m_footer_margin, margin);
	}
	if (Preferences::instance().alwaysShowHeader()) {
		margin = std::max(m_header_margin, margin);
	}
	m_layout->setRowMinimumHeight(0, margin);
	m_layout->setRowMinimumHeight(5, margin);
	m_layout->setColumnMinimumWidth(0, margin);
	m_layout->setColumnMinimumWidth(5, margin);

	int minimum_size = (margin * 2) + (m_theme.foregroundPadding() * 2) + 100;
	window()->setMinimumSize(minimum_size, minimum_size);
}

//-----------------------------------------------------------------------------

void Stack::updateMask()
{
	clearMask();
	raise();

	if (m_scenes->isVisible()) {
		QApplication::processEvents();
		m_scenes->update();
		m_scenes->clearFocus();
		m_scenes->setFocus();
	}
	if (m_header_visible || m_footer_visible) {
		setMask(rect().adjusted(0, m_header_visible, 0, m_footer_visible));
	}
}

//-----------------------------------------------------------------------------

void Stack::updateMenuIndexes()
{
	for (int i = 0; i < m_document_actions.size(); ++i) {
		m_document_actions[i]->setData(i);
	}
}

//-----------------------------------------------------------------------------

void Stack::initPrinter()
{
	if (m_printer) {
		return;
	}

	m_printer = new QPrinter(QPrinter::HighResolution);
	m_printer->setPageSize(QPageSize(QPageSize::Letter));
	m_printer->setPageOrientation(QPageLayout::Portrait);
	m_printer->setPageMargins(QMarginsF(1.0, 1.0, 1.0, 1.0), QPageLayout::Inch);
}

//-----------------------------------------------------------------------------
