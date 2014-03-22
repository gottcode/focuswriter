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

#include "document.h"

#include "alert.h"
#include "block_stats.h"
#include "daily_progress.h"
#include "dictionary_manager.h"
#include "document_watcher.h"
#include "document_writer.h"
#include "format_manager.h"
#include "highlighter.h"
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

#include <QApplication>
#include <QBuffer>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QStyle>
#include <QTextBlock>
#include <QTextEdit>
#include <QTimer>

#include <algorithm>
#include <ctime>

//-----------------------------------------------------------------------------

namespace
{
	QList<int> g_untitled_indexes = QList<int>() << 0;

	class TextEdit : public QTextEdit
	{
	public:
		TextEdit(QWidget* parent = 0)
			: QTextEdit(parent)
		{
		}

	protected:
		virtual bool canInsertFromMimeData(const QMimeData* source) const;
		virtual QMimeData* createMimeDataFromSelection() const;
		virtual void insertFromMimeData(const QMimeData* source);
		virtual void keyPressEvent(QKeyEvent* event);
		virtual void inputMethodEvent(QInputMethodEvent* event);

	private:
		QByteArray mimeToRtf(const QMimeData* source) const;
	};

	bool TextEdit::canInsertFromMimeData(const QMimeData* source) const
	{
		return QTextEdit::canInsertFromMimeData(source) || (source->hasFormat(QLatin1String("text/rtf")) && acceptRichText());
	}

	QMimeData* TextEdit::createMimeDataFromSelection() const
	{
		QMimeData* mime = QTextEdit::createMimeDataFromSelection();
		mime->setData(QLatin1String("text/rtf"), mimeToRtf(mime));
		return mime;
	}

	void TextEdit::insertFromMimeData(const QMimeData* source)
	{
		if (isReadOnly()) {
			return;
		}

		if (acceptRichText()) {
			QByteArray richtext;
			if (source->hasFormat(QLatin1String("text/rtf"))) {
				richtext = source->data(QLatin1String("text/rtf"));
			} else if (source->hasHtml()) {
				richtext = mimeToRtf(source);
			} else {
				QTextEdit::insertFromMimeData(source);
				return;
			}

			RtfReader reader;
			QBuffer buffer(&richtext);
			buffer.open(QIODevice::ReadOnly);
			reader.read(&buffer, textCursor());
			buffer.close();
		} else {
			QTextEdit::insertFromMimeData(source);
		}
	}

	void TextEdit::keyPressEvent(QKeyEvent* event)
	{
		QTextEdit::keyPressEvent(event);

		if (event->key() == Qt::Key_Insert) {
			setOverwriteMode(!overwriteMode());
		} else {
			// Play sound effect
			int key = event->key();
			if (!(event->modifiers().testFlag(Qt::ControlModifier)) &&
					!(event->modifiers().testFlag(Qt::AltModifier)) &&
					!(event->modifiers().testFlag(Qt::MetaModifier))) {
				if ((key == Qt::Key_Return) || (key == Qt::Key_Enter)) {
					Sound::play(Qt::Key_Enter);
				} else if ((key < Qt::Key_Escape) || (key == Qt::Key_unknown)) {
					Sound::play(Qt::Key_Any);
				}
			}
		}
	}

	void TextEdit::inputMethodEvent(QInputMethodEvent* event)
	{
		QTextEdit::inputMethodEvent(event);
		Sound::play(Qt::Key_Any);
	}

	QByteArray TextEdit::mimeToRtf(const QMimeData* source) const
	{
		// Parse HTML
		QTextDocument document;
		if (source->hasHtml()) {
			document.setHtml(source->html());
		} else {
			document.setPlainText(source->text());
		}

		// Convert to RTF
		RtfWriter writer;
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		writer.write(&buffer, &document, false);
		buffer.close();

		return buffer.data();
	}
}

//-----------------------------------------------------------------------------

Document::Document(const QString& filename, DailyProgress* daily_progress, QWidget* parent)
	: QWidget(parent),
	m_cache_outdated(false),
	m_index(0),
	m_always_center(false),
	m_mouse_button_down(false),
	m_rich_text(false),
	m_spacings_loaded(false),
	m_focus_mode(0),
	m_scene_list(0),
	m_dictionary(DictionaryManager::instance().requestDictionary()),
	m_cached_block_count(-1),
	m_cached_current_block(-1),
	m_saved_wordcount(0),
	m_page_type(0),
	m_page_amount(0),
	m_wordcount_type(0),
	m_daily_progress(daily_progress)
{
	setMouseTracking(true);

	m_stats = &m_document_stats;

	m_hide_timer = new QTimer(this);
	m_hide_timer->setInterval(5000);
	m_hide_timer->setSingleShot(true);
	connect(m_hide_timer, SIGNAL(timeout()), this, SLOT(hideMouse()));

	// Set up text area
	m_text = new TextEdit(this);
	m_text->installEventFilter(this);
	m_text->setMouseTracking(true);
	m_text->setFrameStyle(QFrame::NoFrame);
	m_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setTabStopWidth(48);
	m_text->document()->setIndentWidth(48);
	m_text->horizontalScrollBar()->setAttribute(Qt::WA_NoMousePropagation);
	m_text->viewport()->setMouseTracking(true);
	m_text->viewport()->installEventFilter(this);
	connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
	connect(m_text, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
	connect(m_text->document(), SIGNAL(modificationChanged(bool)), this, SIGNAL(modificationChanged(bool)));

	m_scene_model = new SceneModel(m_text, this);

	m_highlighter = new Highlighter(m_text, m_dictionary);
	connect(&DictionaryManager::instance(), SIGNAL(changed()), this, SLOT(dictionaryChanged()));

	// Set filename
	if (!filename.isEmpty()) {
		m_rich_text = FormatManager::isRichText(filename);
		m_filename = QFileInfo(filename).canonicalFilePath();
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
	setScrollBarVisible(false);
	connect(m_scrollbar, SIGNAL(actionTriggered(int)), this, SLOT(scrollBarActionTriggered(int)));
	connect(m_scrollbar, SIGNAL(rangeChanged(int,int)), this, SLOT(scrollBarRangeChanged(int,int)));

	// Lay out window
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(0);
	m_layout->setMargin(0);
	m_layout->addWidget(m_text, 0, 1);
	m_layout->addWidget(m_scrollbar, 0, 2, Qt::AlignRight);

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
		DocumentWriter* writer = new DocumentWriter;
		writer->setType(!m_filename.isEmpty() ? m_filename.section(QLatin1Char('.'), -1) : "odt");
		writer->setEncoding(m_encoding);
		writer->setWriteByteOrderMark(Preferences::instance().writeByteOrderMark());
		writer->setDocument(m_text->document()->clone());
		emit writeCacheFile(this, writer);
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
	writer.setEncoding(m_encoding);
	writer.setWriteByteOrderMark(Preferences::instance().writeByteOrderMark());
	writer.setDocument(m_text->document());
	bool saved = writer.write();
	m_encoding = writer.encoding();
	if (saved) {
		m_cache_outdated = false;
		emit replaceCacheFile(this, m_filename);
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

	QByteArray encoding;
	std::swap(m_filename, filename);
	std::swap(m_encoding, encoding);
	if (!save()) {
		std::swap(m_filename, filename);
		std::swap(m_encoding, encoding);
		return false;
	}

	clearIndex();
	updateSaveLocation();
	m_text->setReadOnly(false);
	m_text->document()->setModified(false);
	emit changedName();
	return true;
}

//-----------------------------------------------------------------------------

bool Document::rename()
{
	// Request new filename
	if (m_filename.isEmpty()) {
		return false;
	}
	QString filename = getSaveFileName(tr("Rename File"));
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
	m_encoding.clear();
	save();
	updateSaveLocation();
	m_text->document()->setModified(false);
	emit changedName();
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
		mbox.setWindowTitle(tr("Reload File"));
		mbox.setText(tr("Reload the file %1 from disk?").arg("<i>" + QFileInfo(m_filename).fileName() + "</i>"));
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
	emit loadStarted(Window::tr("Opening %1").arg(QDir::toNativeSeparators(m_filename)));
	m_text->setReadOnly(true);
	disconnect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));
	disconnect(m_text->document(), SIGNAL(undoCommandAdded()), this, SLOT(undoCommandAdded()));
	m_daily_progress->increaseWordCount(-wordCountDelta());
	loadFile(m_filename, -1);
	emit loadFinished();
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

void Document::print()
{
	QPrinter printer;
	printer.setPageSize(QPrinter::Letter);
	printer.setPageMargins(0.5, 0.5, 0.5, 0.5, QPrinter::Inch);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec() == QDialog::Accepted) {
		bool enabled = m_highlighter->enabled();
		m_highlighter->setEnabled(false);
		m_text->print(&printer);
		if (enabled) {
			m_highlighter->setEnabled(true);
		}
	}
}

//-----------------------------------------------------------------------------

bool Document::loadFile(const QString& filename, int position)
{
	bool loaded = true;

	if (filename.isEmpty()) {
		m_text->setReadOnly(false);

		scrollBarRangeChanged(m_scrollbar->minimum(), m_scrollbar->maximum());

		calculateWordCount();
		connect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));
		connect(m_text->document(), SIGNAL(undoCommandAdded()), this, SLOT(undoCommandAdded()));

		return loaded;
	}

	bool enabled = m_highlighter->enabled();
	m_highlighter->setEnabled(false);

	// Cache contents
	emit replaceCacheFile(this, filename);

	// Fetch reader for file
	FormatReader* reader = 0;
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
	if (reader) {
		QString error;
		reader->read(&file, document);
		file.close();
		m_encoding = reader->encoding();

		if (reader->hasError()) {
			error = reader->errorString();
			loaded = false;
			position = -1;
		}

		if (!loaded) {
			emit alert(new Alert(Alert::Warning, error, QStringList(filename), false));
		}
	}
	document->setUndoRedoEnabled(true);
	document->setModified(false);

	document->blockSignals(false);
	m_text->blockSignals(false);

	m_text->setReadOnly(!m_filename.isEmpty() ? !QFileInfo(m_filename).isWritable() : false);

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

	// Update details
	m_cached_stats.clear();
	calculateWordCount();
	m_saved_wordcount = m_document_stats.wordCount();
	if (enabled) {
		m_highlighter->setEnabled(true);
	}
	connect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));
	connect(m_text->document(), SIGNAL(undoCommandAdded()), this, SLOT(undoCommandAdded()));

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
	QString contrast = (qGray(theme.textColor().rgb()) > 127) ? "black" : "white";
	QColor color = theme.foregroundColor();
	color.setAlpha(theme.foregroundOpacity() * 2.55f);
	m_text_color = theme.textColor();
	m_text_color.setAlpha(255);
	m_text->setStyleSheet(
		QString("QTextEdit { background:rgba(%1,%2,%3,%4); color:rgba(%5,%6,%7,%8); selection-background-color:%9; selection-color:%10; padding:%11px; border-radius:%12px; }")
			.arg(color.red())
			.arg(color.green())
			.arg(color.blue())
			.arg(color.alpha())
			.arg(m_text_color.red())
			.arg(m_text_color.green())
			.arg(m_text_color.blue())
			.arg(m_focus_mode ? "128" : "255")
			.arg(theme.textColor().name())
			.arg(contrast)
			.arg(theme.foregroundPadding())
			.arg(theme.foregroundRounding())
	);
	m_highlighter->setMisspelledColor(theme.misspelledColor());

	// Update spacings
	m_block_format = QTextBlockFormat();
#if (QT_VERSION >= QT_VERSION_CHECK(4,8,0))
	m_block_format.setLineHeight(theme.lineSpacing(), (theme.lineSpacing() == 100) ? QTextBlockFormat::SingleHeight : QTextBlockFormat::ProportionalHeight);
#endif
	m_block_format.setTextIndent(48 * theme.indentFirstLine());
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
	int tab_width = theme.tabWidth();
	m_text->setTabStopWidth(tab_width);
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
	m_text->setCursorWidth(!m_block_cursor ? style()->pixelMetric(QStyle::PM_TextCursorWidth) : m_text->fontMetrics().averageCharWidth());

	int margin = theme.foregroundMargin();
	m_layout->setColumnMinimumWidth(0, margin);
	m_layout->setColumnMinimumWidth(2, margin);
	int foreground_width = theme.foregroundWidth();
	foreground_width = qMin(foreground_width, QApplication::desktop()->availableGeometry().width() - (theme.foregroundMargin() * 2));
	if (theme.foregroundPosition() < 3) {
		m_text->setFixedWidth(foreground_width);

		switch (theme.foregroundPosition()) {
		case 0:
			m_layout->setColumnStretch(0, 0);
			m_layout->setColumnStretch(2, 1);
			break;

		case 2:
			m_layout->setColumnStretch(0, 1);
			m_layout->setColumnStretch(2, 0);
			break;

		case 1:
		default:
			m_layout->setColumnStretch(0, 1);
			m_layout->setColumnStretch(2, 1);
			break;
		};
	} else {
		m_text->setMinimumWidth(100);
		m_text->setMaximumWidth(maximumSize().width());

		m_layout->setColumnStretch(0, 0);
		m_layout->setColumnStretch(2, 0);
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
	m_text->setCursorWidth(!m_block_cursor ? style()->pixelMetric(QStyle::PM_TextCursorWidth) : m_text->fontMetrics().averageCharWidth());
	QFont font = m_text->font();
	font.setStyleStrategy(Preferences::instance().smoothFonts() ? QFont::PreferAntialias : QFont::NoAntialias);
	m_text->setFont(font);

	m_highlighter->setEnabled(!isReadOnly() ? Preferences::instance().highlightMisspelled() : false);

	m_default_format = Preferences::instance().saveFormat();
}

//-----------------------------------------------------------------------------

void Document::setFocusMode(int focus_mode)
{
	m_focus_mode = focus_mode;

	QString style_sheet = m_text->styleSheet();
	int end = style_sheet.lastIndexOf(QChar(')'));
	int start = style_sheet.lastIndexOf(QChar(','), end);
	style_sheet.replace(start + 1, end - start - 1, m_focus_mode ? "128" : "255");
	m_text->setStyleSheet(style_sheet);

	if (m_focus_mode) {
		connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(focusText()));
		connect(m_text, SIGNAL(selectionChanged()), this, SLOT(focusText()));
		connect(m_text, SIGNAL(textChanged()), this, SLOT(focusText()));
		focusText();
	} else {
		disconnect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(focusText()));
		disconnect(m_text, SIGNAL(selectionChanged()), this, SLOT(focusText()));
		disconnect(m_text, SIGNAL(textChanged()), this, SLOT(focusText()));
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
	if (!visible) {
		m_scrollbar->setMask(QRect(-1,-1,1,1));
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

	const QPoint& point = mapFromGlobal(event->globalPos());
	if (rect().contains(point)) {
		emit headerVisible(false);
		emit footerVisible(false);
	}
	if (m_scene_list && !m_scene_list->scenesVisible()) {
		int sidebar_region = qMin(m_scene_list->width(), m_layout->cellRect(0,0).width());
		emit scenesVisible(QRect(0,0, sidebar_region, height()).contains(point));
	}
	setScrollBarVisible(m_scrollbar->rect().contains(m_scrollbar->mapFromGlobal(event->globalPos())));
}

//-----------------------------------------------------------------------------

void Document::centerCursor(bool force)
{
	QRect cursor = m_text->cursorRect();
	QRect viewport = m_text->viewport()->rect();
	if (force || m_always_center || (cursor.bottom() >= viewport.bottom()) || (cursor.top() <= viewport.top())) {
		QPoint offset = viewport.center() - cursor.center();
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

void Document::resizeEvent(QResizeEvent* event)
{
	centerCursor(true);
	QWidget::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void Document::wheelEvent(QWheelEvent* event)
{
	if (event->orientation() == Qt::Vertical) {
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
	emit indentChanged(m_text->textCursor().blockFormat().indent());
	emit alignmentChanged();
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
		selection.cursor.movePosition(QTextCursor::StartOfLine);
		selection.cursor.movePosition(QTextCursor::Up);
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
	QWidget* widget = QApplication::widgetAt(QCursor::pos());
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
	for (QTextBlock i = m_text->document()->begin(); i != m_text->document()->end(); i = i.next()) {
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
		BlockStats temp(0);
		QStringList selection = m_text->textCursor().selectedText().split(QChar::ParagraphSeparator, QString::SkipEmptyParts);
		foreach (const QString& string, selection) {
			temp.update(string);
			m_selected_stats.append(&temp);
		}
		m_selected_stats.calculateWordCount(m_wordcount_type);
		m_selected_stats.calculatePageCount(m_page_type, m_page_amount);
		m_stats = &m_selected_stats;
	} else {
		m_stats = &m_document_stats;
	}
	emit changed();
}

//-----------------------------------------------------------------------------

void Document::undoCommandAdded()
{
	if (!m_old_states.isEmpty()) {
		int steps = m_text->document()->availableUndoSteps();
		QMutableHashIterator<int, QPair<QString, bool> > i(m_old_states);
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
	int steps = m_text->document()->availableUndoSteps();
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
			emit changedName();
		}
		if (m_rich_text != state.second) {
			m_rich_text = state.second;
		}
	}

	// Clear cached stats if amount of blocks or current block has changed
	int block_count = m_text->document()->blockCount();
	int current_block = m_text->textCursor().blockNumber();
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
	BlockStats* stats = 0;
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
	int words = m_document_stats.wordCount();
	calculateWordCount();
	m_daily_progress->increaseWordCount(m_document_stats.wordCount() - words);
	emit changed();
}

//-----------------------------------------------------------------------------

void Document::calculateWordCount()
{
	// Cache stats of the blocks other than the current block
	if (!m_cached_stats.isValid()) {
		m_cached_block_count = m_text->document()->blockCount();
		m_cached_current_block = m_text->textCursor().blockNumber();

		BlockStats* stats = 0;
		for (QTextBlock i = m_text->document()->begin(); i != m_text->document()->end(); i = i.next()) {
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
	QTextBlockUserData* data = m_text->document()->findBlockByNumber(m_cached_current_block).userData();
	if (data) {
		m_document_stats.append(static_cast<BlockStats*>(data));
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
	m_index = g_untitled_indexes.last() + 1;
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
	QString filter = FormatManager::filters(type).join(";;");

	// Prompt for filename
	QString filename;
	while (filename.isEmpty()) {
		QString selected;
		filename = QFileDialog::getSaveFileName(window(), title, m_filename, filter, &selected);
		if (filename.isEmpty()) {
			break;
		}

		// Append file extension
		QString type;
		QRegExp exp("\\*(\\.\\w+)");
		bool append_extension = false;
		QStringList types;
		int index = selected.indexOf(QLatin1Char('(')) + 1;
		while ((index = exp.indexIn(selected, index)) != -1) {
			type = exp.cap(1);
			if (filename.endsWith(type)) {
				append_extension = false;
				break;
			}

			append_extension = true;
			types.append(type);
			index += types.last().length();
		}
		if (append_extension) {
			filename.append(types.first());
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
	bool rich_text = FormatManager::isRichText(filename);
	if (m_rich_text == rich_text) {
		return true;
	}

	// Confirm discarding rich text in plain text files
	if (!rich_text && (QMessageBox::question(window(),
			tr("Question"),
			tr("Saving as plain text will discard all formatting. Discard formatting?"),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No) == QMessageBox::No)) {
		return false;
	}

	// Update rich text status
	setRichText(rich_text);
	return true;
}

//-----------------------------------------------------------------------------

void Document::updateSaveLocation()
{
	QString path = QFileInfo(m_filename).canonicalPath();
	QSettings().setValue("Save/Location", path);
	QDir::setCurrent(path);
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
	int count = keys.count();

	// Find undo states nearest to current
	int steps = m_text->document()->availableUndoSteps();
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
