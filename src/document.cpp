/*
	SPDX-FileCopyrightText: 2009-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "document.h"

#include "alert.h"
#include "block_stats.h"
#include "daily_progress.h"
#include "dictionary_manager.h"
#include "document_watcher.h"
#include "docx_reader.h"
#include "docx_writer.h"
#include "format_manager.h"
#include "highlighter.h"
#include "html_writer.h"
#include "odt_reader.h"
#include "odt_writer.h"
#include "preferences.h"
#include "rtf_reader.h"
#include "rtf_writer.h"
#include "scene_list.h"
#include "scene_model.h"
#include "smart_quotes.h"
#include "sound.h"
#include "spell_checker.h"
#include "theme.h"
#include "window.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QShortcut>
#include <QStyle>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QTimer>

#include <algorithm>
#include <ctime>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

//-----------------------------------------------------------------------------

static inline int cursorWidth()
{
#ifdef Q_OS_WIN
	DWORD width = 1;
	if (SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0))
		return int(width);
	else
#endif
		return -1;
}

//-----------------------------------------------------------------------------

namespace
{

QList<int> g_untitled_indexes{ 0 };

class TextEdit : public QTextEdit
{
public:
	TextEdit(Document* document)
		: QTextEdit(document)
		, m_document(document)
	{
	}

protected:
	bool canInsertFromMimeData(const QMimeData* source) const override;
	QMimeData* createMimeDataFromSelection() const override;
	void insertFromMimeData(const QMimeData* source) override;
	void contextMenuEvent(QContextMenuEvent* event) override;
	bool event(QEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	QByteArray mimeToRtf(const QMimeData* source) const;
	void moveToBlockEnd(QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
	void moveToBlockStart(QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

private:
	Document* m_document;
};

bool TextEdit::canInsertFromMimeData(const QMimeData* source) const
{
	return QTextEdit::canInsertFromMimeData(source)
			|| source->hasFormat(QLatin1String("text/rtf"))
			|| source->hasFormat(QLatin1String("text/richtext"))
			|| source->hasFormat(QLatin1String("application/rtf"))
			|| source->hasFormat(QLatin1String("application/x-qt-windows-mime;value=\"Rich Text Format\""))
			|| source->hasFormat(QLatin1String("application/vnd.oasis.opendocument.text"))
			|| source->hasFormat(QLatin1String("application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
}

QMimeData* TextEdit::createMimeDataFromSelection() const
{
	QMimeData* mime = new QMimeData;

	QTextDocument doc;
	QTextCursor cursor(&doc);
	cursor.insertFragment(textCursor().selection());

	{
		OdtWriter writer;
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		writer.write(&buffer, &doc);
		buffer.close();
		mime->setData(QLatin1String("application/vnd.oasis.opendocument.text"), buffer.data());
	}

	{
		DocxWriter writer;
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		writer.write(&buffer, &doc);
		buffer.close();
		mime->setData(QLatin1String("application/vnd.openxmlformats-officedocument.wordprocessingml.document"), buffer.data());
	}

	{
		RtfWriter writer;
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		writer.write(&buffer, &doc);
		buffer.close();
		mime->setData(QLatin1String("text/rtf"), buffer.data());
		mime->setData(QLatin1String("text/richtext"), buffer.data());
		mime->setData(QLatin1String("application/rtf"), buffer.data());
	}

	{
		HtmlWriter writer;
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		writer.write(&buffer, &doc);
		buffer.close();
		mime->setHtml(buffer.data());
	}

	mime->setText(doc.toPlainText());

	return mime;
}

void TextEdit::insertFromMimeData(const QMimeData* source)
{
	if (isReadOnly()) {
		return;
	}

	QTextDocument document;
	QTextCursor cursor(&document);
	cursor.mergeBlockFormat(textCursor().blockFormat());
	const qsizetype formats = document.allFormats().count();
	if (m_document->isRichText()) {
		cursor = textCursor();
	}

	QByteArray richtext;
	if (source->hasFormat(QLatin1String("application/vnd.oasis.opendocument.text"))) {
		QBuffer buffer;
		buffer.setData(source->data(QLatin1String("application/vnd.oasis.opendocument.text")));
		buffer.open(QIODevice::ReadOnly);
		OdtReader reader;
		reader.read(&buffer, cursor);
	} else if (source->hasFormat(QLatin1String("application/vnd.openxmlformats-officedocument.wordprocessingml.document"))) {
		QBuffer buffer;
		buffer.setData(source->data(QLatin1String("application/vnd.openxmlformats-officedocument.wordprocessingml.document")));
		buffer.open(QIODevice::ReadOnly);
		DocxReader reader;
		reader.read(&buffer, cursor);
	} else if (source->hasFormat(QLatin1String("text/rtf"))) {
		richtext = source->data(QLatin1String("text/rtf"));
	} else if (source->hasFormat(QLatin1String("text/richtext"))) {
		richtext = source->data(QLatin1String("text/richtext"));
	} else if (source->hasFormat(QLatin1String("application/rtf"))) {
		richtext = source->data(QLatin1String("application/rtf"));
	} else if (source->hasFormat(QLatin1String("application/x-qt-windows-mime;value=\"Rich Text Format\""))) {
		richtext = source->data(QLatin1String("application/x-qt-windows-mime;value=\"Rich Text Format\""));
	} else if (source->hasHtml()) {
		richtext = mimeToRtf(source);
	} else {
		QTextEdit::insertFromMimeData(source);
		return;
	}

	if (!richtext.isEmpty()) {
		RtfReader reader;
		QBuffer buffer(&richtext);
		buffer.open(QIODevice::ReadOnly);
		reader.read(&buffer, cursor);
		buffer.close();
	}

	if (!m_document->isRichText()) {
		if (document.allFormats().count() > formats) {
			m_document->setRichText(true);
		}
		textCursor().insertFragment(QTextDocumentFragment(&document));
	}
}

void TextEdit::contextMenuEvent(QContextMenuEvent* event)
{
	QMenu* menu = createStandardContextMenu();
	menu->exec(event->globalPos());
	delete menu;
}

bool TextEdit::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride) {
		QKeyEvent* ke = static_cast<QKeyEvent*>(event);
		if (ke->matches(QKeySequence::Cut)
				|| ke->matches(QKeySequence::Copy)
				|| ke->matches(QKeySequence::Paste)
				|| ke->matches(QKeySequence::Redo)
				|| ke->matches(QKeySequence::Undo)
				|| ke->matches(QKeySequence::SelectAll)
				|| ke->matches(QKeySequence::MoveToEndOfBlock)
				|| ke->matches(QKeySequence::MoveToStartOfBlock)
				|| ke->matches(QKeySequence::SelectEndOfBlock)
				|| ke->matches(QKeySequence::SelectStartOfBlock)) {
			event->ignore();
			return true;
		}
	}
	return QTextEdit::event(event);
}

void TextEdit::keyPressEvent(QKeyEvent* event)
{
	if (event->matches(QKeySequence::Cut)
			|| event->matches(QKeySequence::Copy)
			|| event->matches(QKeySequence::Paste)
			|| event->matches(QKeySequence::Redo)
			|| event->matches(QKeySequence::Undo)
			|| event->matches(QKeySequence::SelectAll)
			|| event->matches(QKeySequence::MoveToEndOfBlock)
			|| event->matches(QKeySequence::MoveToStartOfBlock)
			|| event->matches(QKeySequence::SelectEndOfBlock)
			|| event->matches(QKeySequence::SelectStartOfBlock)) {
		event->ignore();
		return;
	}

#ifndef Q_OS_MAC
	const Qt::KeyboardModifiers move_modifiers = Qt::ControlModifier;
	const Qt::KeyboardModifiers select_modifiers = Qt::ControlModifier | Qt::ShiftModifier;
#else
	const Qt::KeyboardModifiers move_modifiers = Qt::AltModifier;
	const Qt::KeyboardModifiers select_modifiers = Qt::AltModifier | Qt::ShiftModifier;
#endif
	if (event->key() == Qt::Key_Up) {
		if (event->modifiers() == move_modifiers) {
			moveToBlockStart();
			event->ignore();
			return;
		} else if (event->modifiers() == select_modifiers) {
			moveToBlockStart(QTextCursor::KeepAnchor);
			event->ignore();
			return;
		}
	} else if (event->key() == Qt::Key_Down) {
		if (event->modifiers() == move_modifiers) {
			moveToBlockEnd();
			event->ignore();
			return;
		} else if (event->modifiers() == select_modifiers) {
			moveToBlockEnd(QTextCursor::KeepAnchor);
			event->ignore();
			return;
		}
	}

	// Keep formatting in new paragraphs
	if (event->matches(QKeySequence::InsertParagraphSeparator)) {
		Sound::play(Qt::Key_Enter);
		textCursor().insertBlock();
		event->accept();
		ensureCursorVisible();
		return;
	}

	QTextEdit::keyPressEvent(event);

	if (event->key() == Qt::Key_Insert) {
		setOverwriteMode(!overwriteMode());
	} else {
		// Play sound effect
		if (!(event->modifiers().testFlag(Qt::ControlModifier)) &&
				!(event->modifiers().testFlag(Qt::MetaModifier))) {
			Sound::play(Qt::Key_Any);
		}
	}
}

QByteArray TextEdit::mimeToRtf(const QMimeData* source) const
{
	// Parse HTML
	QTextDocument document;
	if (source->hasHtml()) {
		document.setHtml(source->html().remove(QChar(0x0)));
	} else {
		document.setPlainText(source->text().remove(QChar(0x0)));
	}

	// Convert to RTF
	RtfWriter writer;
	QBuffer buffer;
	buffer.open(QIODevice::WriteOnly);
	writer.write(&buffer, &document);
	buffer.close();

	return buffer.data();
}

void TextEdit::moveToBlockEnd(QTextCursor::MoveMode mode)
{
	QTextCursor cursor = textCursor();
	if (cursor.atEnd()) {
		return;
	}

	if (cursor.atBlockEnd()) {
		cursor.movePosition(QTextCursor::NextCharacter, mode);
	}
	cursor.movePosition(QTextCursor::EndOfBlock, mode);

	setTextCursor(cursor);
}

void TextEdit::moveToBlockStart(QTextCursor::MoveMode mode)
{
	QTextCursor cursor = textCursor();
	if (cursor.atStart()) {
		return;
	}

	if (cursor.atBlockStart()) {
		cursor.movePosition(QTextCursor::PreviousCharacter, mode);
	}
	cursor.movePosition(QTextCursor::StartOfBlock, mode);

	setTextCursor(cursor);
}

}

//-----------------------------------------------------------------------------

Document::Document(const QString& filename, DailyProgress* daily_progress, QWidget* parent)
	: QWidget(parent)
	, m_cache_outdated(false)
	, m_index(0)
	, m_always_center(false)
	, m_mouse_button_down(false)
	, m_rich_text(false)
	, m_spacings_loaded(false)
	, m_focus_mode(0)
	, m_scene_list(nullptr)
	, m_dictionary(DictionaryManager::instance().requestDictionary())
	, m_cached_block_count(-1)
	, m_cached_current_block(-1)
	, m_saved_wordcount(0)
	, m_page_type(0)
	, m_page_amount(0)
	, m_wordcount_type(0)
	, m_daily_progress(daily_progress)
{
	setMouseTracking(true);

	m_stats = &m_document_stats;

	m_hide_timer = new QTimer(this);
	m_hide_timer->setInterval(5000);
	m_hide_timer->setSingleShot(true);
	connect(m_hide_timer, &QTimer::timeout, this, &Document::hideMouse);

	// Set up text area
	m_text = new TextEdit(this);
	m_text->installEventFilter(this);
	m_text->setMouseTracking(true);
	m_text->setFrameStyle(QFrame::NoFrame);
	m_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setTabStopDistance(48);
	m_text->document()->setIndentWidth(48);
	m_text->horizontalScrollBar()->setAttribute(Qt::WA_NoMousePropagation);
	m_text->viewport()->setMouseTracking(true);
	m_text->viewport()->installEventFilter(this);
	connect(m_text, &QTextEdit::cursorPositionChanged, this, &Document::cursorPositionChanged);
	connect(m_text, &QTextEdit::selectionChanged, this, &Document::selectionChanged);
	connect(m_text->document(), &QTextDocument::modificationChanged, this, &Document::modificationChanged);

	m_scene_model = new SceneModel(m_text, this);

	m_highlighter = new Highlighter(m_text, m_dictionary);
	connect(&DictionaryManager::instance(), &DictionaryManager::changed, this, &Document::dictionaryChanged);

	// Set filename
	if (!filename.isEmpty()) {
		m_rich_text = FormatManager::isRichText(filename);
		m_filename = QFileInfo(filename).absoluteFilePath();
		updateState();
	}

	if (m_filename.isEmpty()) {
		findIndex();
	}

	// Set up scroll bar
	m_scrollbar = m_text->verticalScrollBar();
	m_scrollbar->setAttribute(Qt::WA_NoMousePropagation);
	m_scrollbar->setPalette(QApplication::palette());
	m_scrollbar->setAutoFillBackground(true);
	m_scrollbar->setMouseTracking(true);
	m_scrollbar->installEventFilter(this);
	setScrollBarVisible(Preferences::instance().alwaysShowScrollBar());
	connect(m_scrollbar, &QScrollBar::actionTriggered, this, &Document::scrollBarActionTriggered);
	connect(m_scrollbar, &QScrollBar::rangeChanged, this, &Document::scrollBarRangeChanged);

	// Lay out window
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->addWidget(m_text, 1, 1);
	m_layout->addWidget(m_scrollbar, 1, 1, 1, 2, Qt::AlignRight);

	// Load settings
	loadPreferences();

	// Make it read-only until content is loaded
	m_text->setReadOnly(true);

	DocumentWatcher::instance()->addWatch(this);
}

//-----------------------------------------------------------------------------

Document::~Document()
{
	m_scene_model->removeAllScenes();

	DocumentWatcher::instance()->removeWatch(this);
	clearIndex();
}

//-----------------------------------------------------------------------------

QString Document::title() const
{
	QString name = QFileInfo(m_filename).fileName();
	if (name.isEmpty()) {
		name = tr("(Untitled %1)").arg(m_index);
	}
	if (isReadOnly()) {
		name = tr("%1 (Read-Only)").arg(name);
	}
	return name;
}

//-----------------------------------------------------------------------------

bool Document::isModified() const
{
	return m_text->document()->isModified();
}

//-----------------------------------------------------------------------------

bool Document::isReadOnly() const
{
	return m_text->isReadOnly();
}

//-----------------------------------------------------------------------------

void Document::cache()
{
	if (m_cache_outdated) {
		m_cache_outdated = false;
		QSharedPointer<DocumentWriter> writer(new DocumentWriter);
		writer->setType(!m_filename.isEmpty() ? m_filename.section(QLatin1Char('.'), -1) : "odt");
		writer->setWriteByteOrderMark(Preferences::instance().writeByteOrderMark());
		writer->setDocument(m_text->document()->clone());
		Q_EMIT writeCacheFile(this, writer);
	}
}

//-----------------------------------------------------------------------------

bool Document::save()
{
	// Save progress
	m_daily_progress->save();

	if (m_filename.isEmpty() || !processFileName(m_filename)) {
		return saveAs();
	}

	// Write file to disk
	DocumentWatcher::instance()->pauseWatch(this);
	DocumentWriter writer;
	writer.setFileName(m_filename);
	writer.setType(m_filename.section(QLatin1Char('.'), -1));
	writer.setWriteByteOrderMark(Preferences::instance().writeByteOrderMark());
	writer.setDocument(m_text->document());
	const bool saved = writer.write();
	if (saved) {
		m_cache_outdated = false;
		Q_EMIT replaceCacheFile(this, m_filename);
	} else {
		cache();
	}
	DocumentWatcher::instance()->resumeWatch(this);

	if (!saved) {
		QMessageBox::critical(window(), tr("Sorry"), tr("Unable to save '%1'.").arg(QDir::toNativeSeparators(m_filename)));
		return false;
	}

	m_saved_wordcount = m_document_stats.wordCount();

	m_text->document()->setModified(false);
	return true;
}

//-----------------------------------------------------------------------------

bool Document::saveAs()
{
	// Request new filename
	QString filename = getSaveFileName(tr("Save File As"));
	if (filename.isEmpty()) {
		return false;
	}
	if (m_filename == filename) {
		return save();
	}

	// Save file as new name
	if (QFile::exists(filename) && (DocumentWatcher::instance()->isWatching(filename) || !QFile::remove(filename))) {
		QMessageBox::critical(window(), tr("Sorry"), tr("Unable to overwrite '%1'.").arg(QDir::toNativeSeparators(filename)));
		return false;
	}

	std::swap(m_filename, filename);
	if (!save()) {
		std::swap(m_filename, filename);
		return false;
	}

	clearIndex();
	updateSaveLocation();
	m_text->setReadOnly(false);
	m_text->document()->setModified(false);
	Q_EMIT changedName();
	return true;
}

//-----------------------------------------------------------------------------

bool Document::rename()
{
	// Request new filename
	if (m_filename.isEmpty()) {
		return false;
	}
	const QString filename = getSaveFileName(tr("Rename File"));
	if (filename.isEmpty()) {
		return false;
	}

	// Rename file
	if (QFile::exists(filename) && (DocumentWatcher::instance()->isWatching(filename) || !QFile::remove(filename))) {
		QMessageBox::critical(window(), tr("Sorry"), tr("Unable to overwrite '%1'.").arg(QDir::toNativeSeparators(filename)));
		return false;
	}
	DocumentWatcher::instance()->pauseWatch(this);
	if (!QFile::rename(m_filename, filename)) {
		DocumentWatcher::instance()->resumeWatch(this);
		QMessageBox::critical(window(), tr("Sorry"), tr("Unable to rename '%1'.").arg(QDir::toNativeSeparators(m_filename)));
		return false;
	}
	DocumentWatcher::instance()->resumeWatch(this);
	m_filename = filename;
	save();
	updateSaveLocation();
	m_text->document()->setModified(false);
	Q_EMIT changedName();
	return true;
}

//-----------------------------------------------------------------------------

void Document::reload(bool prompt)
{
	// Abort if there is no file to reload
	if (m_index) {
		return;
	}

	// Confirm that they do want to reload
	if (prompt) {
		QMessageBox mbox(window());
		mbox.setIcon(QMessageBox::Question);
		mbox.setWindowTitle(tr("Reload File?"));
		mbox.setText(tr("Reload the file '%1' from disk?").arg(QFileInfo(m_filename).fileName()));
		mbox.setInformativeText(tr("All unsaved changes will be lost."));

		QPushButton* reload_button = mbox.addButton(tr("Reload"), QMessageBox::AcceptRole);
		if (reload_button->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
			reload_button->setIcon(reload_button->style()->standardIcon(QStyle::SP_BrowserReload));
		}
		mbox.addButton(QMessageBox::Cancel);
		mbox.setDefaultButton(reload_button);

		if (mbox.exec() == QMessageBox::Cancel) {
			return;
		}
	}

	// Reload file
	Q_EMIT loadStarted(Window::tr("Opening %1").arg(QDir::toNativeSeparators(m_filename)));
	m_text->setReadOnly(true);
	disconnect(m_text->document(), &QTextDocument::contentsChange, this, &Document::updateWordCount);
	disconnect(m_text->document(), &QTextDocument::undoCommandAdded, this, &Document::undoCommandAdded);
	m_daily_progress->increaseWordCount(-wordCountDelta());
	loadFile(m_filename, -1);
	Q_EMIT loadFinished();
}

//-----------------------------------------------------------------------------

void Document::close()
{
	clearIndex();
	deleteLater();
}

//-----------------------------------------------------------------------------

void Document::checkSpelling()
{
	SpellChecker::checkDocument(m_text, m_dictionary);
}

//-----------------------------------------------------------------------------

// Copied and modified from QTextDocument
static void printPage(int index, QPainter *painter, const QTextDocument *doc, const QRectF &body, const QPointF &pageNumberPos)
{
	painter->save();
	painter->translate(body.left(), body.top() - (index - 1) * body.height());
	const QRectF view(0, (index - 1) * body.height(), body.width(), body.height());

	QAbstractTextDocumentLayout *layout = doc->documentLayout();
	QAbstractTextDocumentLayout::PaintContext ctx;

	painter->setClipRect(view);
	ctx.clip = view;

	// don't use the system palette text as default text color, on HP/UX
	// for example that's white, and white text on white paper doesn't
	// look that nice
	ctx.palette.setColor(QPalette::Text, Qt::black);

	layout->draw(painter, ctx);

	if (!pageNumberPos.isNull()) {
		painter->setClipping(false);
		painter->setFont(QFont(doc->defaultFont()));
		const QString pageString = QString::number(index);

		painter->drawText(qRound(pageNumberPos.x() - painter->fontMetrics().boundingRect(pageString).width()),
			qRound(pageNumberPos.y() + view.top()),
			pageString);
	}

	painter->restore();
}

// Copied and modified from QTextDocument
static void printDocument(QPrinter* printer, QTextDocument* doc)
{
	QPainter p(printer);

	// Check that there is a valid device to print to.
	if (!p.isActive())
		return;

	// Make sure that there is a layout
	doc->documentLayout();

	QAbstractTextDocumentLayout *layout = doc->documentLayout();
	layout->setPaintDevice(p.device());

	const int dpiy = p.device()->logicalDpiY();
	QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
	fmt.setMargin(0);
	doc->rootFrame()->setFrameFormat(fmt);

	const qreal pageNumberHeight = QFontMetrics(doc->defaultFont(), p.device()).ascent() + 5 * dpiy / 72.0;
	const QRectF body = QRectF(0, 0, printer->width(), printer->height() - pageNumberHeight);
	const QPointF pageNumberPos = QPointF(body.width(), body.height() + pageNumberHeight);
	doc->setPageSize(body.size());

	int fromPage = printer->fromPage();
	int toPage = printer->toPage();
	const bool ascending = true;

	if (fromPage == 0 && toPage == 0) {
		fromPage = 1;
		toPage = doc->pageCount();
	}
	// paranoia check
	fromPage = qMax(1, fromPage);
	toPage = qMin(doc->pageCount(), toPage);

	if (toPage < fromPage) {
		// if the user entered a page range outside the actual number
		// of printable pages, just return
		return;
	}

	int page = fromPage;
	Q_FOREVER {
		printPage(page, &p, doc, body, pageNumberPos);

		if (page == toPage)
			break;

		if (ascending)
			++page;
		else
			--page;

		if (!printer->newPage())
			return;
	}
}

void Document::print(QPrinter* printer)
{
	QPrintDialog dialog(printer, this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	// Clone document
	QTextDocument* document = m_text->document()->clone();

	// Apply spacings
	const int tab_width = (document->indentWidth() / 96.0) * printer->resolution();
	const bool indent_first_line = !qFuzzyIsNull(document->begin().blockFormat().textIndent());
	QTextBlockFormat block_format;
	block_format.setTextIndent(tab_width * indent_first_line);
	for (int i = 0, count = document->allFormats().count(); i < count; ++i) {
		QTextFormat& f = document->allFormats()[i];
		if (f.isBlockFormat()) {
			f.merge(block_format);
		}
	}
	document->setIndentWidth(tab_width);

	// Apply headings
	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		const int heading = block.blockFormat().property(QTextFormat::UserProperty).toInt();
		if (!heading) {
			continue;
		}

		QTextCharFormat format = block.charFormat();
		format.setProperty(QTextFormat::FontSizeAdjustment, 4 - heading);
		format.setFontWeight(QFont::Bold);

		QTextCursor cursor(document);
		cursor.setPosition(block.position());
		cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		cursor.mergeCharFormat(format);
	}

	// Print document
	printDocument(printer, document);
	delete document;

	// Reset pages
	printer->setFromTo(0, 0);
}

//-----------------------------------------------------------------------------

bool Document::loadFile(const QString& filename, int position)
{
	bool loaded = true;

	if (filename.isEmpty()) {
		m_text->setReadOnly(false);

		scrollBarRangeChanged(m_scrollbar->minimum(), m_scrollbar->maximum());

		calculateWordCount();
		connect(m_text->document(), &QTextDocument::contentsChange, this, &Document::updateWordCount);
		connect(m_text->document(), &QTextDocument::undoCommandAdded, this, &Document::undoCommandAdded);

		return loaded;
	}

	const bool enabled = m_highlighter->enabled();
	m_highlighter->setEnabled(false);

	// Cache contents
	Q_EMIT replaceCacheFile(this, filename);

	// Fetch reader for file
	FormatReader* reader = nullptr;
	QFile file(filename);
	if (file.open(QIODevice::ReadOnly)) {
		reader = FormatManager::createReader(&file, filename.section(QLatin1Char('.'), -1).toLower());
	}

	// Load text area contents
	QTextDocument* document = m_text->document();
	m_text->blockSignals(true);
	document->blockSignals(true);

	document->setUndoRedoEnabled(false);
	document->clear();
	m_text->textCursor().mergeBlockFormat(m_block_format);
	const qsizetype formats = document->allFormats().count();
	if (reader) {
		QString error;
		reader->read(&file, document);
		file.close();

		if (reader->hasError()) {
			error = reader->errorString();
			loaded = false;
			position = -1;
		}

		if (!loaded) {
			Q_EMIT alert(new Alert(Alert::Warning, error, QStringList(filename), false));
			findIndex();
		}
	}
	if (m_filename.isEmpty()) {
		m_rich_text = document->allFormats().count() > formats;
	}
	document->setUndoRedoEnabled(true);
	document->setModified(false);

	document->blockSignals(false);
	m_text->blockSignals(false);

	m_text->setReadOnly(!m_filename.isEmpty() ? !QFileInfo(m_filename).isWritable() : false);

	// Update details
	m_cached_stats.clear();
	calculateWordCount();
	m_saved_wordcount = m_document_stats.wordCount();
	connect(m_text->document(), &QTextDocument::contentsChange, this, &Document::updateWordCount);
	connect(m_text->document(), &QTextDocument::undoCommandAdded, this, &Document::undoCommandAdded);

	// Force highlight before enabling spellcheck to prevent vertical shift from heading elements
	for (QTextBlock i = document->begin(); i.isValid(); i = i.next()) {
		m_highlighter->rehighlightBlock(i);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	m_highlighter->setEnabled(enabled);

	// Restore cursor position
	scrollBarRangeChanged(m_scrollbar->minimum(), m_scrollbar->maximum());
	QTextCursor cursor = m_text->textCursor();
	if (position != -1) {
		cursor.setPosition(position);
	} else {
		cursor.movePosition(QTextCursor::End);
	}
	m_text->setTextCursor(cursor);
	centerCursor(true);

	if (m_focus_mode) {
		focusText();
	}

	if (loaded && !m_filename.isEmpty()) {
		DocumentWatcher::instance()->updateWatch(this);
	}
	return loaded;
}

//-----------------------------------------------------------------------------

void Document::loadTheme(const Theme& theme)
{
	m_text->document()->blockSignals(true);

	// Update colors
	m_text_color = theme.textColor();
	m_text_color.setAlpha(255);
	QColor text_color = m_text_color;
	text_color.setAlpha(m_focus_mode ? 128 : 255);

	QPalette p = m_text->palette();
	p.setBrush(QPalette::Base, Qt::transparent);
	p.setColor(QPalette::Text, text_color);
	p.setColor(QPalette::Highlight, m_text_color);
	p.setColor(QPalette::HighlightedText, (qGray(m_text_color.rgb()) > 127) ? Qt::black : Qt::white);
	m_text->setPalette(p);

	m_highlighter->setMisspelledColor(theme.misspelledColor());

	// Update spacings
	const int tab_width = theme.tabWidth();
	m_block_format = QTextBlockFormat();
	m_block_format.setLineHeight(theme.lineSpacing(), (theme.lineSpacing() == 100) ? QTextBlockFormat::SingleHeight : QTextBlockFormat::ProportionalHeight);
	m_block_format.setTextIndent(tab_width * theme.indentFirstLine());
	m_block_format.setTopMargin(theme.spacingAboveParagraph());
	m_block_format.setBottomMargin(theme.spacingBelowParagraph());
	if (m_spacings_loaded) {
		for (int i = 0, count = m_text->document()->allFormats().count(); i < count; ++i) {
			QTextFormat& f = m_text->document()->allFormats()[i];
			if (f.isBlockFormat()) {
				f.merge(m_block_format);
			}
		}
	} else {
		m_text->setUndoRedoEnabled(false);
		m_text->textCursor().mergeBlockFormat(m_block_format);
		m_text->setUndoRedoEnabled(true);
		m_text->document()->setModified(false);
		m_spacings_loaded = true;
	}

	m_text->setTabStopDistance(tab_width);
	m_text->document()->setIndentWidth(tab_width);

	// Update text
	QFont font = theme.textFont();
	font.setStyleStrategy(m_text->font().styleStrategy());
	if (m_text->font() != font) {
		m_text->setFont(font);
	} else {
		// Force relaying out document so that spacings are updated
		QEvent e(QEvent::FontChange);
		QApplication::sendEvent(m_text, &e);
	}
	m_text->setCursorWidth(!m_block_cursor ? cursorWidth() : m_text->fontMetrics().averageCharWidth());

	const int padding = theme.foregroundPadding();
	m_layout->setRowMinimumHeight(0, padding);
	m_layout->setRowMinimumHeight(2, padding);
	const int margin = theme.foregroundMargin() + padding;
	m_layout->setColumnMinimumWidth(0, margin);
	m_layout->setColumnMinimumWidth(2, margin);
	switch (theme.foregroundPosition()) {
	case 0:
		// Left
		m_layout->setColumnStretch(0, 0);
		m_layout->setColumnStretch(2, 1);
		break;
	case 2:
		// Right
		m_layout->setColumnStretch(0, 1);
		m_layout->setColumnStretch(2, 0);
		break;
	case 3:
		// Stretched
		m_layout->setColumnStretch(0, 0);
		m_layout->setColumnStretch(2, 0);
		break;
	case 1:
	default:
		// Centered
		m_layout->setColumnStretch(0, 1);
		m_layout->setColumnStretch(2, 1);
		break;
	}

	if (m_focus_mode) {
		focusText();
	}

	centerCursor(true);
	m_text->document()->blockSignals(false);
}

//-----------------------------------------------------------------------------

void Document::loadPreferences()
{
	m_always_center = Preferences::instance().alwaysCenter();

	m_page_type = Preferences::instance().pageType();
	switch (m_page_type) {
	case 1:
		m_page_amount = Preferences::instance().pageParagraphs();
		break;
	case 2:
		m_page_amount = Preferences::instance().pageWords();
		break;
	default:
		m_page_amount = Preferences::instance().pageCharacters();
		break;
	}

	m_wordcount_type = Preferences::instance().wordcountType();
	if (m_cached_block_count != -1) {
		calculateWordCount();
	}

	m_block_cursor = Preferences::instance().blockCursor();
	m_text->setCursorWidth(!m_block_cursor ? cursorWidth() : m_text->fontMetrics().averageCharWidth());
	QFont font = m_text->font();
	font.setStyleStrategy(Preferences::instance().smoothFonts() ? QFont::PreferAntialias : QFont::NoAntialias);
	m_text->setFont(font);

	m_highlighter->setEnabled(!isReadOnly() ? Preferences::instance().highlightMisspelled() : false);

	m_default_format = Preferences::instance().saveFormat();

	setScrollBarVisible(Preferences::instance().alwaysShowScrollBar());
}

//-----------------------------------------------------------------------------

void Document::setFocusMode(int focus_mode)
{
	m_focus_mode = focus_mode;

	QColor text_color = m_text_color;
	text_color.setAlpha(m_focus_mode ? 128 : 255);
	QPalette p = m_text->palette();
	p.setColor(QPalette::Text, text_color);
	m_text->setPalette(p);

	if (m_focus_mode) {
		connect(m_text, &QTextEdit::cursorPositionChanged, this, &Document::focusText);
		connect(m_text, &QTextEdit::selectionChanged, this, &Document::focusText);
		connect(m_text, &QTextEdit::textChanged, this, &Document::focusText);
		focusText();
	} else {
		disconnect(m_text, &QTextEdit::cursorPositionChanged, this, &Document::focusText);
		disconnect(m_text, &QTextEdit::selectionChanged, this, &Document::focusText);
		disconnect(m_text, &QTextEdit::textChanged, this, &Document::focusText);
		m_text->setExtraSelections(QList<QTextEdit::ExtraSelection>());
	}
}

//-----------------------------------------------------------------------------

void Document::setRichText(bool rich_text)
{
	if (m_rich_text == rich_text) {
		return;
	}

	updateState();

	// Set file type
	m_rich_text = rich_text;

	// Always remove formatting to have something to undo
	QTextCursor cursor(m_text->document());
	cursor.beginEditBlock();
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	cursor.setBlockFormat(m_block_format);
	cursor.setCharFormat(QTextCharFormat());
	cursor.endEditBlock();
	updateState();
}

//-----------------------------------------------------------------------------

void Document::setModified(bool modified)
{
	m_text->document()->setModified(modified);
}

//-----------------------------------------------------------------------------

void Document::setScrollBarVisible(bool visible)
{
	if (!visible && !Preferences::instance().alwaysShowScrollBar()) {
		m_scrollbar->setMask(QRegion(-1,-1,1,1));
		update();
	} else {
		m_scrollbar->clearMask();
	}
}

//-----------------------------------------------------------------------------

void Document::setSceneList(SceneList* scene_list)
{
	m_scene_list = scene_list;
}

//-----------------------------------------------------------------------------

bool Document::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::MouseMove) {
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
	} else if (event->type() == QEvent::KeyPress && watched == m_text) {
		m_daily_progress->increaseTime();
		if (SmartQuotes::isEnabled() && SmartQuotes::insert(m_text, static_cast<QKeyEvent*>(event))) {
			return true;
		}
	} else if (event->type() == QEvent::Drop) {
		static_cast<Window*>(window())->addDocuments(static_cast<QDropEvent*>(event));
		if (event->isAccepted()) {
			return true;
		}
	} else if (event->type() == QEvent::MouseButtonPress) {
		m_mouse_button_down = true;
		m_scene_list->hideScenes();
	} else if (event->type() == QEvent::MouseButtonRelease) {
		m_mouse_button_down = false;
	} else if (event->type() == QEvent::MouseButtonDblClick) {
		m_mouse_button_down = true;
	}
	return QWidget::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------

void Document::mouseMoveEvent(QMouseEvent* event)
{
	m_text->viewport()->setCursor(Qt::IBeamCursor);
	unsetCursor();
	m_hide_timer->start();

	const QPoint global = event->globalPosition().toPoint();

	const QPoint point = mapFromGlobal(global);
	if (rect().contains(point)) {
		Q_EMIT headerVisible(false);
		Q_EMIT footerVisible(false);
	}
	if (m_scene_list && !m_scene_list->scenesVisible()) {
		const int sidebar_region = std::min(m_scene_list->width(), m_layout->cellRect(0,0).width());
		Q_EMIT scenesVisible(QRect(0,0, sidebar_region, height()).contains(point));
	}
	setScrollBarVisible(m_scrollbar->rect().contains(m_scrollbar->mapFromGlobal(global)));
}

//-----------------------------------------------------------------------------

void Document::centerCursor(bool force)
{
	const QRect cursor = m_text->cursorRect();
	const QRect viewport = m_text->viewport()->rect();
	if (force || m_always_center || (cursor.bottom() >= viewport.bottom()) || (cursor.top() <= viewport.top())) {
		const QPoint offset = viewport.center() - cursor.center();
		QScrollBar* scrollbar = m_text->verticalScrollBar();
		scrollbar->setValue(scrollbar->value() - offset.y());
	}
}

//-----------------------------------------------------------------------------

void Document::mousePressEvent(QMouseEvent* event)
{
	m_scene_list->hideScenes();
	QWidget::mousePressEvent(event);
}

//-----------------------------------------------------------------------------

void Document::wheelEvent(QWheelEvent* event)
{
	if (event->angleDelta().y() != 0) {
		QApplication::sendEvent(m_scrollbar, event);
	} else {
		QApplication::sendEvent(m_text->horizontalScrollBar(), event);
	}
	event->ignore();
	QWidget::wheelEvent(event);
}

//-----------------------------------------------------------------------------

void Document::cursorPositionChanged()
{
	Q_EMIT indentChanged(m_text->textCursor().blockFormat().indent());
	Q_EMIT alignmentChanged();
	if (!m_mouse_button_down) {
		centerCursor();
	}
}

//-----------------------------------------------------------------------------

void Document::focusText()
{
	QTextEdit::ExtraSelection selection;
	selection.format.setForeground(m_text_color);
	selection.cursor = m_text->textCursor();

	switch (m_focus_mode) {
	case 1: // Current line
		selection.cursor.movePosition(QTextCursor::StartOfLine);
		selection.cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
		break;

	case 2: // Current line and previous two lines
		selection.cursor.movePosition(QTextCursor::Up);
		selection.cursor.movePosition(QTextCursor::StartOfLine);
		selection.cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 2);
		selection.cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
		break;

	case 3: // Current paragraph
		selection.cursor.movePosition(QTextCursor::EndOfBlock);
		selection.cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
		break;

	default:
		break;
	}

	QList<QTextEdit::ExtraSelection> selections;
	selections.append(selection);
	m_text->setExtraSelections(selections);
}

//-----------------------------------------------------------------------------

void Document::hideMouse()
{
	const QWidget* widget = QApplication::widgetAt(QCursor::pos());
	if (m_text->viewport()->hasFocus() && (widget == m_text->viewport() || widget == this)) {
		m_text->viewport()->setCursor(Qt::BlankCursor);
		setCursor(Qt::BlankCursor);
	}
}

//-----------------------------------------------------------------------------

void Document::scrollBarActionTriggered(int action)
{
	if (action == QAbstractSlider::SliderToMinimum) {
		m_text->moveCursor(QTextCursor::Start);
	} else if (action == QAbstractSlider::SliderToMaximum) {
		m_text->moveCursor(QTextCursor::End);
	}
}

//-----------------------------------------------------------------------------

void Document::scrollBarRangeChanged(int, int max)
{
	m_scrollbar->blockSignals(true);
	m_scrollbar->setMaximum(max + m_text->viewport()->height());
	m_scrollbar->blockSignals(false);
}

//-----------------------------------------------------------------------------

void Document::dictionaryChanged()
{
	for (QTextBlock i = m_text->document()->begin(); i.isValid(); i = i.next()) {
		if (i.userData()) {
			static_cast<BlockStats*>(i.userData())->update(i.text());
		}
	}
	m_highlighter->updateSpelling();
}

//-----------------------------------------------------------------------------

void Document::selectionChanged()
{
	m_selected_stats.clear();
	if (m_text->textCursor().hasSelection()) {
		BlockStats temp(nullptr);
		const QStringList selection = m_text->textCursor().selectedText().split(QChar::ParagraphSeparator, Qt::SkipEmptyParts);
		for (const QString& string : selection) {
			temp.update(string);
			m_selected_stats.append(&temp);
		}
		m_selected_stats.calculateWordCount(m_wordcount_type);
		m_selected_stats.calculatePageCount(m_page_type, m_page_amount);
		m_stats = &m_selected_stats;
	} else {
		m_stats = &m_document_stats;
	}
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

void Document::undoCommandAdded()
{
	if (!m_old_states.isEmpty()) {
		const int steps = m_text->document()->availableUndoSteps();
		QMutableHashIterator<int, QPair<QString, bool>> i(m_old_states);
		while (i.hasNext()) {
			i.next();
			if (i.key() >= steps) {
				i.remove();
			}
		}
	}
}

//-----------------------------------------------------------------------------

void Document::updateWordCount(int position, int removed, int added)
{
	m_cache_outdated = true;

	// Change filename and rich text status if necessary because of undo/redo
	const int steps = m_text->document()->availableUndoSteps();
	if (m_old_states.contains(steps)) {
		const QPair<QString, bool>& state = m_old_states[steps];
		if (m_filename != state.first) {
			m_filename = state.first;
			if (m_filename.isEmpty()) {
				findIndex();
			} else {
				clearIndex();
			}
			DocumentWatcher::instance()->updateWatch(this);
			Q_EMIT changedName();
		}
		if (m_rich_text != state.second) {
			m_rich_text = state.second;
		}
	}

	// Clear cached stats if amount of blocks or current block has changed
	const int block_count = m_text->document()->blockCount();
	const int current_block = m_text->textCursor().blockNumber();
	if (m_cached_block_count != block_count || m_cached_current_block != current_block) {
		m_cached_stats.clear();
	}

	// Update stats of blocks that were modified
	QTextBlock begin = m_text->document()->findBlock(position - removed);
	if (!begin.isValid()) {
		begin = m_text->document()->begin();
	}
	QTextBlock end = m_text->document()->findBlock(position + added);
	if (end.isValid()) {
		end = end.next();
	}
	bool update_spelling = false;
	BlockStats* stats = nullptr;
	for (QTextBlock i = begin; i != end; i = i.next()) {
		stats = static_cast<BlockStats*>(i.userData());
		if (!stats) {
			stats = new BlockStats(m_scene_model);
			i.setUserData(stats);
			m_cached_stats.clear();
			update_spelling = true;
		}
		stats->update(i.text());
		stats->recheckSpelling();
		m_scene_model->updateScene(stats, i);
	}
	if (update_spelling) {
		m_highlighter->updateSpelling();
	}

	// Update document stats and daily word count
	const int words = m_document_stats.wordCount();
	calculateWordCount();
	m_daily_progress->increaseWordCount(m_document_stats.wordCount() - words);
	Q_EMIT changed();
}

//-----------------------------------------------------------------------------

void Document::calculateWordCount()
{
	// Cache stats of the blocks other than the current block
	if (!m_cached_stats.isValid()) {
		m_cached_block_count = m_text->document()->blockCount();
		m_cached_current_block = m_text->textCursor().blockNumber();

		BlockStats* stats = nullptr;
		for (QTextBlock i = m_text->document()->begin(); i.isValid(); i = i.next()) {
			if (!i.userData()) {
				stats = new BlockStats(m_scene_model);
				i.setUserData(stats);
				stats->update(i.text());
				m_scene_model->updateScene(stats, i);
			}
			if (i.blockNumber() != m_cached_current_block) {
				m_cached_stats.append(static_cast<BlockStats*>(i.userData()));
			}
		}
	}

	// Determine document stats by adding cached stats to current block stats
	m_document_stats = m_cached_stats;
	const QTextBlockUserData* data = m_text->document()->findBlockByNumber(m_cached_current_block).userData();
	if (data) {
		m_document_stats.append(static_cast<const BlockStats*>(data));
	}
	m_document_stats.calculateWordCount(m_wordcount_type);
	m_document_stats.calculatePageCount(m_page_type, m_page_amount);
}

//-----------------------------------------------------------------------------

void Document::clearIndex()
{
	if (m_index) {
		g_untitled_indexes.removeAll(m_index);
		m_index = 0;
	}
}

//-----------------------------------------------------------------------------

void Document::findIndex()
{
	m_index = g_untitled_indexes.constLast() + 1;
	g_untitled_indexes.append(m_index);
}

//-----------------------------------------------------------------------------

QString Document::getSaveFileName(const QString& title)
{
	// Determine filter
	QString type = m_filename.section(QLatin1Char('.'), -1).toLower();
	if (type.isEmpty()) {
		type = m_default_format;
	}
	const QString filter = FormatManager::filters(type).join(";;");

	// Determine location
	QString path = m_filename;
	if (m_filename.isEmpty()) {
		const QString default_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
		path = QSettings().value("Save/Location", default_path).toString() + "/" + tr("Untitled %1").arg(m_index);
	}

	// Prompt for filename
	QString filename;
	while (filename.isEmpty()) {
		QString selected;
		filename = QFileDialog::getSaveFileName(window(), title, path, filter, &selected);
		if (filename.isEmpty()) {
			break;
		}

		// Append file extension
		static const QRegularExpression regex("\\*(\\.\\w+)");
		QRegularExpressionMatchIterator i = regex.globalMatch(selected);
		bool append_extension = i.hasNext();
		QStringList types;
		while (i.hasNext()) {
			const QString type = i.next().captured(1);
			types << type;

			if (filename.endsWith(type)) {
				append_extension = false;
				break;
			}
		}
		if (append_extension) {
			filename.append(types.constFirst());
		}

		// Handle rich text in plain text file
		if (!processFileName(filename)) {
			filename.clear();
		}
	}

	return filename;
}

//-----------------------------------------------------------------------------

bool Document::processFileName(const QString& filename)
{
	// Check if rich text status is the same
	const bool rich_text = FormatManager::isRichText(filename);
	if (m_rich_text == rich_text) {
		return true;
	}

	// Confirm discarding rich text in plain text files
	if (!rich_text && (QMessageBox::question(window(),
			tr("Question"),
			tr("Saving as plain text will discard all formatting. Discard formatting?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)) {
		return false;
	}

	// Update rich text status
	setRichText(rich_text);
	return true;
}

//-----------------------------------------------------------------------------

void Document::updateSaveLocation()
{
	const QString path = QFileInfo(m_filename).absolutePath();
	QSettings().setValue("Save/Location", path);
	updateState();
	updateSaveName();
}

//-----------------------------------------------------------------------------

void Document::updateSaveName()
{
	// List undo states
	if (m_old_states.isEmpty()) {
		return;
	}
	QList<int> keys = m_old_states.keys();
	std::sort(keys.begin(), keys.end());
	const int count = keys.count();

	// Find undo states nearest to current
	const int steps = m_text->document()->availableUndoSteps();
	int nearest_smaller = 0;
	int nearest_larger = count - 1;
	for (int i = 0; i < count; ++i) {
		if (keys.at(i) <= steps) {
			nearest_smaller = i;
		}
		if (keys.at(i) >= steps) {
			nearest_larger = i;
			break;
		}
	}

	// Replace filename of states until the rich text status differs
	for (int i = nearest_smaller; i > -1; --i) {
		QPair<QString, bool>& state = m_old_states[keys.at(i)];
		if (state.second == m_rich_text) {
			state.first = m_filename;
		} else {
			break;
		}
	}
	for (int i = nearest_larger; i < count; ++i) {
		QPair<QString, bool>& state = m_old_states[keys.at(i)];
		if (state.second == m_rich_text) {
			state.first = m_filename;
		} else {
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void Document::updateState()
{
	m_old_states[m_text->document()->availableUndoSteps()] = qMakePair(m_filename, m_rich_text);
}

//-----------------------------------------------------------------------------
