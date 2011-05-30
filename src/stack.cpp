/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "alert_layer.h"
#include "document.h"
#include "find_dialog.h"
#include "load_screen.h"
#include "smart_quotes.h"
#include "theme.h"

#include <QFileInfo>
#include <QGridLayout>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextCursor>
#include <QTextStream>
#include <QThread>
#include <QTimer>

//-----------------------------------------------------------------------------

namespace
{
	class BackgroundLoader : public QThread
	{
	public:
		void create(int position, const QString& image_path, QWidget* widget);
		QPixmap pixmap();
		void reset();

	protected:
		virtual void run();

	private:
		struct File {
			int position;
			QString image_path;
			QRect rect;
			QPalette palette;
		};
		QList<File> m_files;
		QMutex m_file_mutex;
		QImage m_image;
		QMutex m_image_mutex;
	} background_loader;

	void BackgroundLoader::create(int position, const QString& image_path, QWidget* widget)
	{
		File file = { position, image_path, widget->rect(), widget->palette() };

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

			QImage image = Theme::renderBackground(file.image_path, file.position, file.palette.color(QPalette::Window), file.rect.size());

			m_image_mutex.lock();
			m_image = image;
			m_image_mutex.unlock();

			m_file_mutex.lock();
		} while (!m_files.isEmpty());
		m_file_mutex.unlock();
	}
}

//-----------------------------------------------------------------------------

Stack::Stack(QWidget* parent)
	: QWidget(parent),
	m_current_document(0),
	m_background_position(0),
	m_margin(0),
	m_footer_margin(0),
	m_header_margin(0),
	m_footer_visible(0),
	m_header_visible(0)
{
	setMouseTracking(true);

	m_contents = new QStackedWidget(this);

	m_alerts = new AlertLayer(this);

	m_load_screen = new LoadScreen(this);

	m_find_dialog = new FindDialog(this);
	connect(m_find_dialog, SIGNAL(findNextAvailable(bool)), this, SIGNAL(findNextAvailable(bool)));

	m_layout = new QGridLayout(this);
	m_layout->setMargin(0);
	m_layout->setSpacing(0);
	m_layout->setRowMinimumHeight(1, 6);
	m_layout->setRowMinimumHeight(4, 6);
	m_layout->setRowStretch(2, 1);
	m_layout->setColumnMinimumWidth(1, 6);
	m_layout->setColumnMinimumWidth(4, 6);
	m_layout->setColumnStretch(2, 2);
	m_layout->setColumnStretch(3, 1);
	m_layout->addWidget(m_contents, 1, 0, 4, 6);
	m_layout->addWidget(m_alerts, 3, 3);
	m_layout->addWidget(m_load_screen, 0, 0, 6, 6);

	m_resize_timer = new QTimer(this);
	m_resize_timer->setInterval(50);
	m_resize_timer->setSingleShot(true);
	connect(m_resize_timer, SIGNAL(timeout()), this, SLOT(updateBackground()));
	connect(&background_loader, SIGNAL(finished()), this, SLOT(updateBackground()));
}

//-----------------------------------------------------------------------------

Stack::~Stack()
{
	background_loader.wait();
}

//-----------------------------------------------------------------------------

void Stack::addDocument(Document* document)
{
	connect(document, SIGNAL(alignmentChanged()), this, SIGNAL(updateFormatAlignmentActions()));
	connect(document, SIGNAL(changedName()), this, SIGNAL(updateFormatActions()));
	connect(document, SIGNAL(changedName()), this, SLOT(updateMapping()));
	connect(document, SIGNAL(formattingEnabled(bool)), this, SIGNAL(formattingEnabled(bool)));
	connect(document, SIGNAL(footerVisible(bool)), this, SLOT(setFooterVisible(bool)));
	connect(document, SIGNAL(headerVisible(bool)), this, SLOT(setHeaderVisible(bool)));
	connect(document->text(), SIGNAL(copyAvailable(bool)), this, SIGNAL(copyAvailable(bool)));
	connect(document->text(), SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
	connect(document->text(), SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
	connect(document->text(), SIGNAL(currentCharFormatChanged(const QTextCharFormat&)), this, SIGNAL(updateFormatActions()));

	m_documents.append(document);
	m_contents->addWidget(document);
	m_contents->setCurrentWidget(document);
	updateMapping();

	emit documentAdded(document);
	emit formattingEnabled(document->isRichText());
	emit updateFormatActions();
}

//-----------------------------------------------------------------------------

void Stack::moveDocument(int from, int to)
{
	m_documents.move(from, to);
	updateMapping();
}

//-----------------------------------------------------------------------------

void Stack::removeDocument(int index)
{
	Document* document = m_documents.takeAt(index);
	m_contents->removeWidget(document);
	emit documentRemoved(document);
	document->deleteLater();
	updateMapping();
}

//-----------------------------------------------------------------------------

void Stack::setCurrentDocument(int index)
{
	m_current_document = m_documents[index];
	m_contents->setCurrentWidget(m_current_document);

	emit copyAvailable(!m_current_document->text()->textCursor().selectedText().isEmpty());
	emit redoAvailable(m_current_document->text()->document()->isRedoAvailable());
	emit undoAvailable(m_current_document->text()->document()->isUndoAvailable());
	emit formattingEnabled(m_current_document->isRichText());
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
	m_current_document->text()->setAlignment(Qt::AlignCenter);
}

//-----------------------------------------------------------------------------

void Stack::alignJustify()
{
	m_current_document->text()->setAlignment(Qt::AlignJustify);
}

//-----------------------------------------------------------------------------

void Stack::alignLeft()
{
	m_current_document->text()->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
}

//-----------------------------------------------------------------------------

void Stack::alignRight()
{
	m_current_document->text()->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
}

//-----------------------------------------------------------------------------

void Stack::autoCache()
{
	foreach (Document* document, m_documents) {
		if (document->text()->document()->isModified()) {
			document->cache();
		}
	}
}

//-----------------------------------------------------------------------------

void Stack::autoSave()
{
	foreach (Document* document, m_documents) {
		if (document->text()->document()->isModified()) {
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
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat format = cursor.blockFormat();
	format.setIndent(format.indent() - 1);
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
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat format = cursor.blockFormat();
	format.setIndent(format.indent() + 1);
	cursor.setBlockFormat(format);
	emit updateFormatActions();
}

//-----------------------------------------------------------------------------

void Stack::makePlainText()
{
	if (!m_current_document->text()->document()->isEmpty()
		&& QMessageBox::warning(window(), tr("Question"), tr("Remove all formatting from the current file?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}
	m_current_document->setRichText(false);
}

//-----------------------------------------------------------------------------

void Stack::makeRichText()
{
	m_current_document->setRichText(true);
}

//-----------------------------------------------------------------------------

void Stack::paste()
{
	m_current_document->text()->paste();
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

void Stack::replace()
{
	m_find_dialog->showReplaceMode();
}

//-----------------------------------------------------------------------------

void Stack::save()
{
	m_current_document->save();
	updateMapping();
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

void Stack::setFontBold(bool bold)
{
	m_current_document->text()->setFontWeight(bold ? QFont::Bold : QFont::Normal);
}

//-----------------------------------------------------------------------------

void Stack::setFontItalic(bool italic)
{
	m_current_document->text()->setFontItalic(italic);
}

//-----------------------------------------------------------------------------

void Stack::setFontStrikeOut(bool strikeout)
{
	QTextCharFormat format;
	format.setFontStrikeOut(strikeout);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

//-----------------------------------------------------------------------------

void Stack::setFontUnderline(bool underline)
{
	m_current_document->text()->setFontUnderline(underline);
}

//-----------------------------------------------------------------------------

void Stack::setFontSuperScript(bool super)
{
	QTextCharFormat format;
	format.setVerticalAlignment(super ? QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

//-----------------------------------------------------------------------------

void Stack::setFontSubScript(bool sub)
{
	QTextCharFormat format;
	format.setVerticalAlignment(sub ? QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

//-----------------------------------------------------------------------------

void Stack::setTextDirectionLTR()
{
	if (m_current_document) {
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
		QTextCursor cursor = m_current_document->text()->textCursor();
		QTextBlockFormat format = cursor.blockFormat();
		format.setLayoutDirection(Qt::RightToLeft);
		format.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
		cursor.mergeBlockFormat(format);
		emit updateFormatAlignmentActions();
	}
}

//-----------------------------------------------------------------------------

void Stack::themeSelected(const Theme& theme)
{
	m_background_position = theme.backgroundType();
	m_background_path = theme.backgroundImage();

	QPalette p = palette();
	p.setColor(QPalette::Window, theme.backgroundColor().rgb());
	setPalette(p);

	background_loader.reset();
	m_background = QPixmap();
	m_background_old = QPixmap();
	updateBackground();

	m_margin = theme.foregroundMargin();
	m_layout->setRowMinimumHeight(0, m_margin);
	m_layout->setRowMinimumHeight(5, m_margin);
	m_layout->setColumnMinimumWidth(0, m_margin);
	m_layout->setColumnMinimumWidth(5, m_margin);

	foreach (Document* document, m_documents) {
		document->loadTheme(theme);
	}

	window()->setMinimumWidth((m_margin * 2) + theme.foregroundWidth());
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
	if (!m_background.isNull()) {
		painter.drawPixmap(event->rect(), m_background, event->rect());
	}
	painter.end();
}

//-----------------------------------------------------------------------------

void Stack::resizeEvent(QResizeEvent* event) {
	updateMask();
	if (!m_background_old.isNull() && m_background_old.size() == size()) {
		qSwap(m_background, m_background_old);
	} else {
		if (!m_background.isNull()) {
			m_background_old = m_background;
			m_background = QPixmap();
		}
		m_resize_timer->start();
		updateBackground();
	}
	QWidget::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void Stack::updateBackground()
{
	m_background = background_loader.pixmap();
	if ((m_background.isNull() || m_background.size() != size()) && isVisible()) {
		m_background = QPixmap();
		background_loader.create(m_background_position, m_background_path, this);
		setAttribute(Qt::WA_NoSystemBackground, false);
		setAutoFillBackground(true);
	} else {
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAutoFillBackground(false);
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
	}
}

//-----------------------------------------------------------------------------

void Stack::updateMapping()
{
	QFile file(Document::cachePath() + "/mapping");
	if (file.open(QFile::WriteOnly | QFile::Text)) {
		QTextStream stream(&file);
		stream.setCodec(QTextCodec::codecForName("UTF-8"));
		stream.setGenerateByteOrderMark(true);
		foreach (Document* document, m_documents) {
			stream << QFileInfo(document->cacheFilename()).baseName() << " " << document->filename() << endl;
		}
	}
}

//-----------------------------------------------------------------------------
