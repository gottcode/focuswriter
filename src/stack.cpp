/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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
#include "alert_layer.h"
#include "document.h"
#include "find_dialog.h"
#include "load_screen.h"
#include "scene_list.h"
#include "scene_model.h"
#include "smart_quotes.h"
#include "symbols_dialog.h"
#include "theme.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopWidget>
#include <QDir>
#include <QGridLayout>
#include <QMenu>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPaintEvent>
#include <QStackedWidget>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QThread>
#include <QTimer>

//-----------------------------------------------------------------------------

namespace
{
	class BackgroundLoader : public QThread
	{
	public:
		void create(const Theme& theme, const QSize& background);
		QPixmap pixmap();
		void reset();

	protected:
		virtual void run();

	private:
		struct File {
			Theme theme;
			QSize background;
		};
		QList<File> m_files;
		QMutex m_file_mutex;

		struct CacheFile {
			File file;
			QRect foreground;
			QImage image;
			bool operator==(const CacheFile& other);
		};
		QList<CacheFile> m_cache;

		QImage m_image;
		QMutex m_image_mutex;
	} background_loader;

	void BackgroundLoader::create(const Theme& theme, const QSize& background)
	{
		File file = {
			theme,
			background
		};

		m_file_mutex.lock();
		m_files.append(file);
		m_file_mutex.unlock();

		if (!isRunning()) {
			start();
		}
	}

	QPixmap BackgroundLoader::pixmap()
	{
		QMutexLocker locker(&m_image_mutex);
		return QPixmap::fromImage(m_image, Qt::AutoColor | Qt::AvoidDither);
	}

	void BackgroundLoader::reset()
	{
		QMutexLocker locker(&m_image_mutex);
		m_image = QImage();
	}

	void BackgroundLoader::run()
	{
		m_file_mutex.lock();
		do {
			File file = m_files.takeLast();
			m_files.clear();
			m_file_mutex.unlock();

			CacheFile cache_file = { file, QRect(), QImage() };
			int index = m_cache.indexOf(cache_file);
			if (index != -1) {
				cache_file = m_cache.at(index);
			} else {
				cache_file.image = file.theme.renderBackground(file.background);
				cache_file.image = file.theme.renderForeground(cache_file.image, file.background, cache_file.foreground);
				m_cache.prepend(cache_file);
				while (m_cache.size() > 10) {
					m_cache.removeLast();
				}
			}

			m_image_mutex.lock();
			m_image = cache_file.image;
			m_image_mutex.unlock();

			m_file_mutex.lock();
		} while (!m_files.isEmpty());
		m_file_mutex.unlock();
	}

	bool BackgroundLoader::CacheFile::operator==(const CacheFile& other)
	{
		return (file.theme == other.file.theme) && (file.background == other.file.background);
	}
}

//-----------------------------------------------------------------------------

Stack::Stack(QWidget* parent) :
	QWidget(parent),
	m_symbols_dialog(0),
	m_current_document(0),
	m_margin(0),
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
	connect(m_menu_group, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));

	m_load_screen = new LoadScreen(this);

	m_find_dialog = new FindDialog(this);
	connect(m_find_dialog, SIGNAL(findNextAvailable(bool)), this, SIGNAL(findNextAvailable(bool)));

	connect(ActionManager::instance(), SIGNAL(insertText(QString)), this, SLOT(insertSymbol(QString)));

	m_layout = new QGridLayout(this);
	m_layout->setMargin(0);
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
	m_layout->addWidget(m_load_screen, 0, 0, 6, 6);

	m_resize_timer = new QTimer(this);
	m_resize_timer->setInterval(50);
	m_resize_timer->setSingleShot(true);
	connect(m_resize_timer, SIGNAL(timeout()), this, SLOT(updateBackground()));
	connect(&background_loader, SIGNAL(finished()), this, SLOT(updateBackground()));

	// Always draw background
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAutoFillBackground(false);
	updateBackground();
}

//-----------------------------------------------------------------------------

Stack::~Stack()
{
	background_loader.wait();
}

//-----------------------------------------------------------------------------

void Stack::addDocument(Document* document)
{
	document->setSceneList(m_scenes);
	connect(document, SIGNAL(alert(Alert*)), m_alerts, SLOT(addAlert(Alert*)));
	connect(document, SIGNAL(alignmentChanged()), this, SIGNAL(updateFormatAlignmentActions()));
	connect(document, SIGNAL(changedName()), this, SIGNAL(updateFormatActions()));
	connect(document, SIGNAL(footerVisible(bool)), this, SLOT(setFooterVisible(bool)));
	connect(document, SIGNAL(headerVisible(bool)), this, SLOT(setHeaderVisible(bool)));
	connect(document, SIGNAL(scenesVisible(bool)), this, SLOT(setScenesVisible(bool)));
	connect(document->text(), SIGNAL(copyAvailable(bool)), this, SIGNAL(copyAvailable(bool)));
	connect(document->text(), SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
	connect(document->text(), SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
	connect(document->text(), SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SIGNAL(updateFormatActions()));

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
	updateMask();
	showHeader();
}

//-----------------------------------------------------------------------------

void Stack::waitForThemeBackground()
{
	if (background_loader.isRunning()) {
		background_loader.wait();
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
	foreach (Document* document, m_documents) {
		if (document->isModified()) {
			document->cache();
		}
	}
}

//-----------------------------------------------------------------------------

void Stack::autoSave()
{
	foreach (Document* document, m_documents) {
		if (document->isModified()) {
			if (!document->filename().isEmpty()) {
				document->save();
			} else {
				document->cache();
			}
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
	format.setIndent(qMax(0, format.indent() - 1));
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

void Stack::print()
{
	m_current_document->print();
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
	foreach (Document* document, m_documents) {
		document->setFocusMode(focus_mode);
	}
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
		connect(m_symbols_dialog, SIGNAL(insertText(QString)), this, SLOT(insertSymbol(QString)));
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

	QPalette p = palette();
	p.setColor(QPalette::Window, theme.backgroundColor().rgb());
	setPalette(p);

	background_loader.reset();
	updateBackground();

	m_margin = theme.foregroundMargin();
	m_layout->setRowMinimumHeight(0, m_margin);
	m_layout->setRowMinimumHeight(5, m_margin);
	m_layout->setColumnMinimumWidth(0, m_margin);
	m_layout->setColumnMinimumWidth(5, m_margin);

	if (m_symbols_dialog) {
		m_symbols_dialog->setPreviewFont(theme.textFont());
	}

	int minimum_size = (m_margin * 2) + 100;
	if (theme.foregroundPosition() < 3) {
		window()->setMinimumWidth(qMin((m_margin * 2) + theme.foregroundWidth(), QApplication::desktop()->availableGeometry().width()));
	} else {
		window()->setMinimumWidth(minimum_size);
	}
	window()->setMinimumHeight(minimum_size);

	foreach (Document* document, m_documents) {
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
	painter.drawPixmap(event->rect(), m_background, event->rect());
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
	m_background = background_loader.pixmap();
	if (m_background.isNull() || m_background.size() != size()) {
		m_background = QPixmap(size());
		m_background.fill(palette().color(QPalette::Window));

		background_loader.create(m_theme, size());
	}
	update();
}

//-----------------------------------------------------------------------------

void Stack::updateMask()
{
	if (m_header_visible || m_footer_visible) {
		setMask(rect().adjusted(0, m_header_visible, 0, m_footer_visible));
		setAttribute(Qt::WA_TransparentForMouseEvents, true);
	} else {
		clearMask();
		setAttribute(Qt::WA_TransparentForMouseEvents, false);
		raise();

		if (m_scenes->isVisible()) {
			QApplication::processEvents();
			m_scenes->update();
			m_scenes->clearFocus();
			m_scenes->setFocus();
		}
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
