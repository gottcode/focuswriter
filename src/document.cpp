/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "block_stats.h"
#include "dictionary_manager.h"
#include "document_writer.h"
#include "highlighter.h"
#include "odt_reader.h"
#include "preferences.h"
#include "smart_quotes.h"
#include "sound.h"
#include "spell_checker.h"
#include "theme.h"
#include "window.h"
#include "rtf/reader.h"
#include "rtf/writer.h"

#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollBar>
#include <QSettings>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>

#include <ctime>

//-----------------------------------------------------------------------------

namespace
{
	QList<int> g_untitled_indexes = QList<int>() << 0;

	QString g_cache_path;

	QString randomCacheFilename()
	{
		static time_t seed = 0;
		if (seed == 0) {
			seed = time(0);
			qsrand(seed);
		}

		QString filename;
		QDir dir(g_cache_path);
		do {
			filename = QString("fw_%1").arg(qrand(), 6, 36, QLatin1Char('0'));
		} while (dir.exists(filename));
		return filename;
	}

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

			RTF::Reader reader;
			QBuffer buffer(&richtext);
			buffer.open(QIODevice::ReadOnly);
			reader.read(&buffer, textCursor());
			buffer.close();
		} else {
			QTextEdit::insertFromMimeData(source);
		}
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
		RTF::Writer writer;
		QBuffer buffer;
		buffer.open(QIODevice::WriteOnly);
		writer.write(&buffer, &document, false);
		buffer.close();

		return buffer.data();
	}
}

//-----------------------------------------------------------------------------

Document::Document(const QString& filename, int& current_wordcount, int& current_time, QWidget* parent)
	: QWidget(parent),
	m_cache_filename(randomCacheFilename()),
	m_cache_outdated(false),
	m_index(0),
	m_always_center(false),
	m_rich_text(false),
	m_focus_mode(0),
	m_cached_block_count(-1),
	m_cached_current_block(-1),
	m_page_type(0),
	m_page_amount(0),
	m_accurate_wordcount(true),
	m_current_wordcount(current_wordcount),
	m_current_time(current_time)
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
	m_text->document()->setIndentWidth(1);
	m_text->horizontalScrollBar()->setAttribute(Qt::WA_NoMousePropagation);
	m_text->viewport()->setMouseTracking(true);
	m_text->viewport()->installEventFilter(this);
	connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
	connect(m_text, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

	m_dictionary = DictionaryManager::instance().requestDictionary();
	m_highlighter = new Highlighter(m_text, m_dictionary);
	connect(&DictionaryManager::instance(), SIGNAL(changed()), this, SLOT(dictionaryChanged()));

	// Set filename
	bool unknown_rich_text = false;
	if (!filename.isEmpty()) {
		QString suffix = filename.section(QLatin1Char('.'), -1).toLower();
		m_rich_text = (suffix == "odt") || (suffix == "rtf");
		m_filename = QFileInfo(filename).canonicalFilePath();
		updateState();
	}

	if (m_filename.isEmpty()) {
		findIndex();
		unknown_rich_text = true;
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
	Preferences preferences;
	if (unknown_rich_text) {
		m_rich_text = preferences.richText();
	}
	m_text->setAcceptRichText(m_rich_text);
	loadPreferences(preferences);

	// Make it read-only until content is loaded
	m_text->setReadOnly(true);
}

//-----------------------------------------------------------------------------

Document::~Document()
{
	clearIndex();
	emit removeCacheFile(g_cache_path + m_cache_filename);
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
		writer->setFileName(g_cache_path + m_cache_filename);
		writer->setType(m_filename.section(QLatin1Char('.'), -1));
		writer->setRichText(m_rich_text);
		writer->setCodePage(m_codepage);
		writer->setDocument(m_text->document()->clone());
		emit cacheFile(writer);
	}
}

//-----------------------------------------------------------------------------

bool Document::save()
{
	// Save progress
	QSettings settings;
	settings.setValue("Progress/Words", m_current_wordcount);
	settings.setValue("Progress/Time", m_current_time);

	if (m_filename.isEmpty()) {
		return saveAs();
	}

	// Write file to disk
	DocumentWriter writer;
	writer.setFileName(m_filename);
	writer.setType(m_filename.section(QLatin1Char('.'), -1));
	writer.setRichText(m_rich_text);
	writer.setCodePage(m_codepage);
	writer.setDocument(m_text->document());
	bool saved = writer.write();
	m_codepage = writer.codePage();
	if (saved) {
		m_cache_outdated = false;
		QFile::remove(g_cache_path + m_cache_filename);
		QFile::copy(m_filename, g_cache_path + m_cache_filename);
	} else {
		cache();
	}

	if (!saved) {
		QMessageBox::critical(window(), tr("Sorry"), tr("Unable to save '%1'.").arg(QDir::toNativeSeparators(m_filename)));
		return false;
	}

	m_text->document()->setModified(false);
	return true;
}

//-----------------------------------------------------------------------------

bool Document::saveAs()
{
	QString selected;
	QString filename = QFileDialog::getSaveFileName(window(), tr("Save File As"), m_filename, fileFilter(m_filename), &selected);
	if (!filename.isEmpty()) {
		filename = fileNameWithExtension(filename, selected);
		if (QFile::exists(filename) && !QFile::remove(filename)) {
			QMessageBox::critical(window(), tr("Sorry"), tr("Unable to overwrite '%1'.").arg(QDir::toNativeSeparators(filename)));
			return false;
		}
		qSwap(m_filename, filename);
		if (!save()) {
			m_filename = filename;
			return false;
		}
		clearIndex();
		updateSaveLocation();
		m_text->setReadOnly(false);
		m_text->document()->setModified(false);
		emit changedName();
		return true;
	} else {
		return false;
	}
}

//-----------------------------------------------------------------------------

bool Document::rename()
{
	if (m_filename.isEmpty()) {
		return false;
	}

	QString selected;
	QString filename = QFileDialog::getSaveFileName(window(), tr("Rename File"), m_filename, fileFilter(m_filename), &selected);
	if (!filename.isEmpty()) {
		filename = fileNameWithExtension(filename, selected);
		if (QFile::exists(filename) && !QFile::remove(filename)) {
			QMessageBox::critical(window(), tr("Sorry"), tr("Unable to overwrite '%1'.").arg(QDir::toNativeSeparators(filename)));
			return false;
		}
		if (!QFile::rename(m_filename, filename)) {
			QMessageBox::critical(window(), tr("Sorry"), tr("Unable to rename '%1'.").arg(QDir::toNativeSeparators(m_filename)));
			return false;
		}
		m_filename = filename;
		updateSaveLocation();
		m_text->document()->setModified(false);
		emit changedName();
		return true;
	} else {
		return false;
	}
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
	QFile::copy(filename, g_cache_path + m_cache_filename);

	// Load text area contents
	QTextDocument* document = m_text->document();
	m_text->blockSignals(true);
	document->blockSignals(true);

	document->setUndoRedoEnabled(false);
	if (!m_rich_text) {
		QFile file(filename);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec("UTF-8");
			stream.setAutoDetectUnicode(true);

			QTextCursor cursor(document);
			cursor.beginEditBlock();
			while (!stream.atEnd()) {
				cursor.insertText(stream.read(8192));
				QApplication::processEvents();
			}
			cursor.endEditBlock();
			file.close();
		}
	} else {
		if (m_filename.endsWith(".odt")) {
			ODT::Reader reader;
			reader.read(filename, document);
			if (reader.hasError()) {
				QMessageBox::warning(this, tr("Sorry"), reader.errorString());
				loaded = false;
				position = -1;
			}
		} else {
			QFile file(filename);
			if (file.open(QIODevice::ReadOnly)) {
				RTF::Reader reader;
				QTextCursor cursor(document);
				cursor.movePosition(QTextCursor::End);
				reader.read(&file, cursor);
				m_codepage = reader.codePage();
				file.close();
				if (reader.hasError()) {
					QMessageBox::warning(this, tr("Sorry"), reader.errorString());
					loaded = false;
					position = -1;
				}
			}
		}
	}
	document->setUndoRedoEnabled(true);
	document->setModified(false);

	document->blockSignals(false);
	m_text->blockSignals(false);

	m_text->setReadOnly(!QFileInfo(m_filename).isWritable());

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
	if (enabled) {
		m_highlighter->setEnabled(true);
	}
	connect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));
	connect(m_text->document(), SIGNAL(undoCommandAdded()), this, SLOT(undoCommandAdded()));

	if (m_focus_mode) {
		focusText();
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

	if (m_focus_mode) {
		focusText();
	}

	// Update spacings
	for (int i = 0, count = m_text->document()->allFormats().count(); i < count; ++i) {
		QTextFormat& format = m_text->document()->allFormats()[i];
		if (format.isBlockFormat()) {
#if QT_VERSION >= 0x040800
			if (theme.lineSpacing() == 100) {
				format.setProperty(QTextFormat::LineHeightType, QTextBlockFormat::SingleHeight);
				format.setProperty(QTextFormat::LineHeight, 100.0);
			} else {
				format.setProperty(QTextFormat::LineHeightType, QTextBlockFormat::ProportionalHeight);
				format.setProperty(QTextFormat::LineHeight, static_cast<double>(theme.lineSpacing()));
			}
#endif
			format.setProperty(QTextFormat::TextIndent, 48.0 * theme.indentFirstLine());
			format.setProperty(QTextFormat::BlockTopMargin, static_cast<double>(theme.spacingAboveParagraph()));
			format.setProperty(QTextFormat::BlockBottomMargin, static_cast<double>(theme.spacingBelowParagraph()));
		}
	}

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
	m_text->setCursorWidth(!m_block_cursor ? 1 : m_text->fontMetrics().averageCharWidth());

	int margin = theme.foregroundMargin();
	m_layout->setColumnMinimumWidth(0, margin);
	m_layout->setColumnMinimumWidth(2, margin);
	if (theme.foregroundPosition() < 3) {
		m_text->setFixedWidth(theme.foregroundWidth());

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
		m_text->setMinimumWidth(theme.foregroundWidth());
		m_text->setMaximumWidth(maximumSize().height());

		m_layout->setColumnStretch(0, 0);
		m_layout->setColumnStretch(2, 0);
	}

	centerCursor(true);
	m_text->document()->blockSignals(false);
}

//-----------------------------------------------------------------------------

void Document::loadPreferences(const Preferences& preferences)
{
	m_always_center = preferences.alwaysCenter();

	m_page_type = preferences.pageType();
	switch (m_page_type) {
	case 1:
		m_page_amount = preferences.pageParagraphs();
		break;
	case 2:
		m_page_amount = preferences.pageWords();
		break;
	default:
		m_page_amount = preferences.pageCharacters();
		break;
	}

	m_accurate_wordcount = preferences.accurateWordcount();
	if (m_cached_block_count != -1) {
		calculateWordCount();
	}

	m_block_cursor = preferences.blockCursor();
	m_text->setCursorWidth(!m_block_cursor ? 1 : m_text->fontMetrics().averageCharWidth());
	QFont font = m_text->font();
	font.setStyleStrategy(preferences.smoothFonts() ? QFont::PreferAntialias : QFont::NoAntialias);
	m_text->setFont(font);

	m_highlighter->setEnabled(!isReadOnly() ? preferences.highlightMisspelled() : false);
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
		connect(m_text, SIGNAL(textChanged()), this, SLOT(focusText()));
		focusText();
	} else {
		disconnect(m_text, SIGNAL(textChanged()), this, SLOT(focusText()));
		m_text->setExtraSelections(QList<QTextEdit::ExtraSelection>());
	}
}

//-----------------------------------------------------------------------------

void Document::setRichText(bool rich_text)
{
	// Get new file name
	m_old_states[m_text->document()->availableUndoSteps()] = qMakePair(m_filename, m_rich_text);
	if (!m_filename.isEmpty()) {
		QString filename = m_filename;
		int suffix_index = filename.lastIndexOf(QChar('.'));
		int file_index = filename.lastIndexOf(QChar('/'));
		if (suffix_index > file_index) {
			filename.chop(filename.length() - suffix_index);
		}
		filename.append(rich_text ? ".rtf" : ".txt");
		QString selected;
		m_filename = QFileDialog::getSaveFileName(window(), tr("Save File As"), filename, fileFilter(filename), &selected);
		if (!m_filename.isEmpty()) {
			m_filename = fileNameWithExtension(m_filename, selected);
			clearIndex();
		} else {
			findIndex();
		}
	}

	// Set file type
	m_rich_text = rich_text;
	m_text->setAcceptRichText(m_rich_text);

	// Always remove formatting to have something to undo
	QTextCursor cursor(m_text->document());
	cursor.beginEditBlock();
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	cursor.setBlockFormat(QTextBlockFormat());
	cursor.setCharFormat(QTextCharFormat());
	cursor.endEditBlock();
	m_old_states[m_text->document()->availableUndoSteps()] = qMakePair(m_filename, m_rich_text);

	// Save file
	if (!m_filename.isEmpty()) {
		save();
		updateState();
		m_text->document()->setModified(false);
	}
	emit changedName();
	emit formattingEnabled(m_rich_text);
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

bool Document::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::MouseMove) {
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
	} else if (event->type() == QEvent::KeyPress && watched == m_text) {
		int msecs = m_time.restart();
		if (msecs < 30000) {
			m_current_time += msecs;
		}
		if (SmartQuotes::isEnabled() && SmartQuotes::insert(m_text, static_cast<QKeyEvent*>(event))) {
			return true;
		}
	} else if (event->type() == QEvent::Drop) {
		static_cast<Window*>(window())->addDocuments(static_cast<QDropEvent*>(event));
		if (event->isAccepted()) {
			return true;
		}
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
	setScrollBarVisible(m_scrollbar->rect().contains(m_scrollbar->mapFromGlobal(event->globalPos())));
}

//-----------------------------------------------------------------------------

QString Document::cachePath()
{
	return g_cache_path;
}

//-----------------------------------------------------------------------------

void Document::setCachePath(const QString& path)
{
	g_cache_path = path;
	if (!g_cache_path.endsWith(QLatin1Char('/'))) {
		g_cache_path += QLatin1Char('/');
	}
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
	if (QApplication::mouseButtons() == Qt::NoButton) {
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
		selection.cursor.movePosition(QTextCursor::EndOfLine);
		selection.cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
		break;

	case 2: // Current line and previous two lines
		selection.cursor.movePosition(QTextCursor::EndOfLine);
		selection.cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor, 2);
		selection.cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
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
			static_cast<BlockStats*>(i.userData())->checkSpelling(i.text(), &m_dictionary);
		}
	}
	m_highlighter->rehighlight();
}

//-----------------------------------------------------------------------------

void Document::selectionChanged()
{
	m_selected_stats.clear();
	if (m_text->textCursor().hasSelection()) {
		BlockStats temp("", 0);
		QStringList selection = m_text->textCursor().selectedText().split(QChar::ParagraphSeparator, QString::SkipEmptyParts);
		foreach (const QString& string, selection) {
			temp.update(string, 0);
			m_selected_stats.append(&temp);
		}
		if (!m_accurate_wordcount) {
			m_selected_stats.calculateEstimatedWordCount();
		}
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
			emit changedName();
		}
		if (m_rich_text != state.second) {
			m_rich_text = state.second;
			m_text->setAcceptRichText(m_rich_text);
			emit formattingEnabled(m_rich_text);
		}
	}

	// Play a sound on keypress
	int block_count = m_text->document()->blockCount();
	if (added) {
		if ((m_cached_block_count < block_count) && (m_cached_block_count > 0)) {
			Sound::play(Qt::Key_Enter);
		} else {
			Sound::play(Qt::Key_Any);
		}
	}

	// Clear cached stats if amount of blocks or current block has changed
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
	for (QTextBlock i = begin; i != end; i = i.next()) {
		if (i.userData()) {
			static_cast<BlockStats*>(i.userData())->update(i.text(), &m_dictionary);
		}
	}

	// Update document stats and daily word count
	int words = m_document_stats.wordCount();
	calculateWordCount();
	m_current_wordcount += (m_document_stats.wordCount() - words);
	emit changed();
}

//-----------------------------------------------------------------------------

void Document::calculateWordCount()
{
	// Cache stats of the blocks other than the current block
	if (!m_cached_stats.isValid()) {
		m_cached_block_count = m_text->document()->blockCount();
		m_cached_current_block = m_text->textCursor().blockNumber();

		for (QTextBlock i = m_text->document()->begin(); i != m_text->document()->end(); i = i.next()) {
			if (!i.userData()) {
				i.setUserData(new BlockStats(i.text(), &m_dictionary));
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
	if (!m_accurate_wordcount) {
		m_document_stats.calculateEstimatedWordCount();
	}
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

QString Document::fileFilter(const QString& filename) const
{
	QString plaintext = tr("Plain Text (*.txt)");
	QString opendocumenttext = tr("OpenDocument Text (*.odt)");
	QString richtext = tr("Rich Text (*.rtf)");
	QString all = tr("All Files (*)");
	if (!filename.isEmpty()) {
		QString suffix = filename.section(QLatin1Char('.'), -1).toLower();
		if (suffix == "odt") {
			return opendocumenttext + ";;" + richtext;
		} else if (suffix == "rtf") {
			return richtext + ";;" + opendocumenttext;
		} else if (suffix == "txt") {
			return plaintext + ";;" + all;
		} else {
			return all;
		}
	} else {
		return m_rich_text ? (richtext + ";;" + opendocumenttext) : plaintext;
	}
}

//-----------------------------------------------------------------------------

QString Document::fileNameWithExtension(const QString& filename, const QString& filter) const
{
	QString suffix;
	QRegExp exp("\\*(\\.\\w+)");
	int index = exp.indexIn(filter);
	if (index != -1) {
		suffix = exp.cap(1);
	}

	QString result = filename;
	if (!result.endsWith(suffix)) {
		result.append(suffix);
	}
	return result;
}

//-----------------------------------------------------------------------------

void Document::updateSaveLocation()
{
	QString path = QFileInfo(m_filename).canonicalPath();
	QSettings().setValue("Save/Location", path);
	QDir::setCurrent(path);
	updateState();
}

//-----------------------------------------------------------------------------

void Document::updateState()
{
	if (!m_old_states.isEmpty()) {
		QList<int> keys = m_old_states.keys();
		qSort(keys);
		int count = keys.count();

		int steps = m_text->document()->availableUndoSteps();
		int nearest_smaller = 0;
		int nearest_larger = count - 1;
		for (int i = 0; i < count; ++i) {
			if (keys[i] <= steps) {
				nearest_smaller = i;
			}
			if (keys[i] >= steps) {
				nearest_larger = i;
				break;
			}
		}

		for (int i = nearest_smaller; i > -1; --i) {
			if (m_old_states[i].second == m_rich_text) {
				m_old_states[i].first = m_filename;
			} else {
				break;
			}
		}
		for (int i = nearest_larger; i < count; ++i) {
			if (m_old_states[i].second == m_rich_text) {
				m_old_states[i].first = m_filename;
			} else {
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
