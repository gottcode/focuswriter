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

#include "document.h"

#include "block_stats.h"
#include "dictionary.h"
#include "highlighter.h"
#include "focusmode.h"
#include "preferences.h"
#include "smart_quotes.h"
#include "sound.h"
#include "spell_checker.h"
#include "theme.h"
#include "window.h"
#include "rtf/reader.h"
#include "rtf/writer.h"

#include <QApplication>
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
#include <QTextCodec>
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
			filename = QString("fw_%1").arg(qrand(), 6, 36);
		} while (dir.exists(filename));
		return filename;
	}

	bool isRichTextFile(const QString& filename)
	{
		return filename.endsWith(QLatin1String(".rtf"));
	}
}

//-----------------------------------------------------------------------------

Document::Document(const QString& filename, int& current_wordcount, int& current_time, QWidget* parent)
	: QWidget(parent),
	m_cache_filename(randomCacheFilename()),
	m_index(0),
	m_always_center(false),
	m_rich_text(false),
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
	m_text = new QTextEdit(this);
	m_text->installEventFilter(this);
	m_text->setMouseTracking(true);
	m_text->setFrameStyle(QFrame::NoFrame);
	m_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setTabStopWidth(50);
	m_text->document()->setIndentWidth(50);
	m_text->horizontalScrollBar()->setAttribute(Qt::WA_NoMousePropagation);
	m_text->viewport()->setMouseTracking(true);
	m_text->viewport()->installEventFilter(this);
	connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
	connect(m_text, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
	connect(m_text->document(), SIGNAL(undoCommandAdded()), this, SLOT(undoCommandAdded()));
	connect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));

	m_dictionary = new Dictionary(this);
	m_highlighter = new Highlighter(m_text, m_dictionary);
	m_focusmode = new FocusMode(m_text);
	connect(m_dictionary, SIGNAL(changed()), this, SLOT(dictionaryChanged()));

	// Set filename
	bool unknown_rich_text = false;
	if (!filename.isEmpty()) {
		m_rich_text = isRichTextFile(filename.toLower());
		m_filename = QFileInfo(filename).canonicalFilePath();
		updateState();
	}

	if (m_filename.isEmpty()) {
		findIndex();
		unknown_rich_text = true;
	} else {
		m_text->setReadOnly(!QFileInfo(m_filename).isWritable());
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
}

//-----------------------------------------------------------------------------

Document::~Document()
{
	clearIndex();
	QFile::remove(g_cache_path + m_cache_filename);
}

//-----------------------------------------------------------------------------

bool Document::isReadOnly() const
{
	return m_text->isReadOnly();
}

//-----------------------------------------------------------------------------

void Document::cache()
{
	writeFile(g_cache_path + m_cache_filename);
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
	bool saved = writeFile(m_filename);
	if (saved) {
		QFile::remove(g_cache_path + m_cache_filename);
		QFile::copy(m_filename, g_cache_path + m_cache_filename);
	} else {
		cache();
	}

	if (!saved) {
		QMessageBox::critical(window(), tr("Sorry"), tr("Unable to save '%1'.").arg(m_filename));
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
			QMessageBox::critical(window(), tr("Sorry"), tr("Unable to overwrite '%1'.").arg(filename));
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
			QMessageBox::critical(window(), tr("Sorry"), tr("Unable to overwrite '%1'.").arg(filename));
			return false;
		}
		if (!QFile::rename(m_filename, filename)) {
			QMessageBox::critical(window(), tr("Sorry"), tr("Unable to rename '%1'.").arg(m_filename));
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
	SpellChecker::checkDocument(m_text);
}

//-----------------------------------------------------------------------------

void Document::print()
{
	QPrinter printer;
	printer.setPageSize(QPrinter::Letter);
	printer.setPageMargins(0.5, 0.5, 0.5, 0.5, QPrinter::Inch);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec() == QDialog::Accepted) {
		m_text->print(&printer);
	}
}

//-----------------------------------------------------------------------------

void Document::loadFile(const QString& filename, int position)
{
	if (filename.isEmpty()) {
		return;
	}

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
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			stream.setAutoDetectUnicode(true);

			QTextCursor cursor(document);
			while (!stream.atEnd()) {
				cursor.insertText(stream.read(8192));
				QApplication::processEvents();
			}
			file.close();
		}
	} else {
		QFile file(filename);
		if (file.open(QIODevice::ReadOnly)) {
			RTF::Reader reader;
			reader.read(&file, document);
			m_codepage = reader.codePage();
			file.close();
			if (reader.hasError()) {
				QMessageBox::warning(this, tr("Sorry"), reader.errorString());
			}
		}
	}
	document->setUndoRedoEnabled(true);
	document->setModified(false);

	document->blockSignals(false);
	m_text->blockSignals(false);

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
	m_highlighter->rehighlight();
}

//-----------------------------------------------------------------------------

void Document::loadTheme(const Theme& theme)
{
	m_text->document()->blockSignals(true);

	// Update colors
	QString contrast = (qGray(theme.textColor().rgb()) > 127) ? "black" : "white";
	QColor color = theme.foregroundColor();
	color.setAlpha(theme.foregroundOpacity() * 2.55f);
	m_text->setStyleSheet(
		QString("QTextEdit { background: rgba(%1, %2, %3, %4); color: %5; selection-background-color: %6; selection-color: %7; padding: %8px; border-radius: %9px; }")
			.arg(color.red())
			.arg(color.green())
			.arg(color.blue())
			.arg(color.alpha())
			.arg(theme.textColor().name())
			.arg(theme.textColor().name())
			.arg(contrast)
			.arg(theme.foregroundPadding())
			.arg(theme.foregroundRounding())
	);
	m_highlighter->setMisspelledColor(theme.misspelledColor());
	m_focusmode->setBlurredTextColor(theme.blurredTextColor());

	// Update text
	QFont font = theme.textFont();
	font.setStyleStrategy(m_text->font().styleStrategy());
	if (m_text->font() != font) {
		m_text->setFont(font);
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
	calculateWordCount();

	m_block_cursor = preferences.blockCursor();
	m_text->setCursorWidth(!m_block_cursor ? 1 : m_text->fontMetrics().averageCharWidth());
	QFont font = m_text->font();
	font.setStyleStrategy(preferences.smoothFonts() ? QFont::PreferAntialias : QFont::NoAntialias);
	m_text->setFont(font);

	m_highlighter->setEnabled(!isReadOnly() ? preferences.highlightMisspelled() : false);
	m_focusmode->setLevel(0);
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
			static_cast<BlockStats*>(i.userData())->checkSpelling(i.text(), m_dictionary);
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

	int block_count = m_text->document()->blockCount();
	if (added) {
		if ((m_cached_block_count < block_count) && (m_cached_block_count > 0)) {
			Sound::play(Qt::Key_Enter);
		} else {
			Sound::play(Qt::Key_Any);
		}
	}
	int current_block = m_text->textCursor().blockNumber();
	if (m_cached_block_count != block_count || m_cached_current_block != current_block) {
		m_cached_block_count = block_count;
		m_cached_current_block = current_block;
		m_cached_stats.clear();
	}

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
			static_cast<BlockStats*>(i.userData())->update(i.text(), m_dictionary);
		}
	}

	int words = m_document_stats.wordCount();
	calculateWordCount();
	m_current_wordcount += (m_document_stats.wordCount() - words);
	emit changed();
}

//-----------------------------------------------------------------------------

void Document::calculateWordCount()
{
	if (!m_cached_stats.isValid()) {
		for (QTextBlock i = m_text->document()->begin(); i != m_text->document()->end(); i = i.next()) {
			if (!i.userData()) {
				i.setUserData(new BlockStats(i.text(), m_dictionary));
			}
			if (i.blockNumber() != m_cached_current_block) {
				m_cached_stats.append(static_cast<BlockStats*>(i.userData()));
			}
		}
	}

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
	QString plaintext = tr("Plain Text (*.txt);;All Files (*)");
	QString richtext = tr("Rich Text (*.rtf)");
	QString all = tr("All Files (*)");
	if (!filename.isEmpty()) {
		if (isRichTextFile(filename)) {
			return richtext;
		} else if (filename.endsWith(".txt")) {
			return plaintext;
		} else {
			return all;
		}
	} else {
		return m_rich_text ? richtext : plaintext;
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

bool Document::writeFile(const QString& filename)
{
	bool saved = false;
	QFile file(filename);
	if (!m_rich_text) {
		if (file.open(QFile::WriteOnly | QFile::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			stream.setGenerateByteOrderMark(true);
			stream << m_text->toPlainText();
			saved = true;
		}
	} else {
		if (file.open(QFile::WriteOnly)) {
			RTF::Writer writer(m_codepage);
			if (m_codepage.isEmpty()) {
				m_codepage = writer.codePage();
			}
			saved = writer.write(&file, m_text->document());
		}
	}
	if (file.isOpen()) {
		saved = saved && (file.error() == QFile::NoError);
		file.close();
	}
	return saved;
}

//-----------------------------------------------------------------------------
