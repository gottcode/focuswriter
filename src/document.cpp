/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
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

#include "find_dialog.h"
#include "highlighter.h"
#include "preferences.h"
#include "spell_checker.h"
#include "theme.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QMouseEvent>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollBar>
#include <QSettings>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>
#include <QtConcurrentRun>

#include <cmath>

/*****************************************************************************/

namespace {
	// Text block statistics
	class BlockStats : public QTextBlockUserData {
	public:
		BlockStats(const QString& text) : m_characters(0), m_spaces(0), m_words(0) {
			update(text);
		}

		bool isEmpty() const {
			return m_words == 0;
		}

		int characterCount() const {
			return m_characters;
		}

		int spaceCount() const {
			return m_spaces;
		}

		int wordCount() const {
			return m_words;
		}

		void update(const QString& text);

	private:
		int m_characters;
		int m_spaces;
		int m_words;
	};

	void BlockStats::update(const QString& text) {
		m_characters = text.length();
		m_spaces = 0;
		m_words = 0;
		int index = -1;
		for (int i = 0; i < m_characters; ++i) {
			const QChar& c = text[i];
			if (c.isLetterOrNumber()) {
				if (index == -1) {
					index = i;
					m_words++;
				}
			} else if (c != 0x0027 && c != 0x2019) {
				index = -1;
				m_spaces += c.isSpace();
			}
		}
	}

	// Threaded file saving
	void write(const QString& filename, const QString& data) {
		QFile file(filename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			stream << data;
		}
	}
}

/*****************************************************************************/

Document::Document(const QString& filename, int& current_wordcount, int& current_time, const QSize& size, int margin, QWidget* parent)
: QWidget(parent),
  m_auto_append(true),
  m_margin(50),
  m_character_count(0),
  m_page_count(0),
  m_paragraph_count(0),
  m_space_count(0),
  m_wordcount(0),
  m_page_type(0),
  m_page_amount(0),
  m_accurate_wordcount(true),
  m_current_wordcount(current_wordcount),
  m_current_time(current_time) {
	setMouseTracking(true);
	resize(size);

	m_hide_timer = new QTimer(this);
	m_hide_timer->setInterval(5000);
	m_hide_timer->setSingleShot(true);
	connect(m_hide_timer, SIGNAL(timeout()), this, SLOT(hideMouse()));

	// Set up text area
	m_text = new QPlainTextEdit(this);
	m_text->setCenterOnScroll(true);
	m_text->setTabStopWidth(50);
	m_text->setMinimumHeight(500);
	m_text->setFrameStyle(QFrame::NoFrame);
	m_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->viewport()->setMouseTracking(true);
	m_text->viewport()->installEventFilter(this);
	m_highlighter = new Highlighter(m_text);

	m_scrollbar = m_text->verticalScrollBar();
	m_scrollbar->setPalette(QApplication::palette());
	m_scrollbar->setAutoFillBackground(true);
	m_scrollbar->setVisible(false);

	// Set up find dialog
	m_find_dialog = new FindDialog(m_text, this);

	// Lay out window
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(0);
	m_layout->setMargin(0);
	m_layout->setColumnStretch(0, 1);
	m_layout->setColumnStretch(2, 1);
	m_layout->addWidget(m_text, 1, 1);
	m_layout->addWidget(m_scrollbar, 1, 2, Qt::AlignRight);
	setMargin(margin);

	// Load settings
	Preferences preferences;
	loadPreferences(preferences);
	loadTheme(Theme(QSettings().value("ThemeManager/Theme").toString()));

	// Open file
	if (!filename.isEmpty()) {
		QFile file(filename);
		if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			m_text->setPlainText(stream.readAll());
			m_text->moveCursor(QTextCursor::End);
			m_text->document()->setModified(false);
			file.close();

			m_filename = QFileInfo(file).canonicalFilePath();
			updateSaveLocation();
		}
	}
	if (m_filename.isEmpty()) {
		static int indexes = 0;
		m_index = ++indexes;
	}

	calculateWordCount();
	connect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));
}

/*****************************************************************************/

Document::~Document() {
	m_file_save.waitForFinished();
}

/*****************************************************************************/

bool Document::save() {
	// Save progress
	QSettings settings;
	settings.setValue("Progress/Words", m_current_wordcount);
	settings.setValue("Progress/Time", m_current_time);

	// Write file to disk
	if (!m_filename.isEmpty()) {
		m_file_save.waitForFinished();
		m_file_save = QtConcurrent::run(write, m_filename, m_text->toPlainText());
	} else {
		if (!saveAs()) {
			return false;
		}
	}
	m_text->document()->setModified(false);
	return true;
}

/*****************************************************************************/

bool Document::saveAs() {
	QString filename = QFileDialog::getSaveFileName(this, tr("Save File As"), QString(), tr("Plain Text (*.txt);;All Files (*)"));
	if (!filename.isEmpty()) {
		if (m_auto_append && !filename.endsWith(".txt")) {
			filename.append(".txt");
		}
		m_filename = filename;
		save();
		updateSaveLocation();
		return true;
	} else {
		return false;
	}
}

/*****************************************************************************/

bool Document::rename() {
	if (m_filename.isEmpty()) {
		return false;
	}

	QString filename = QFileDialog::getSaveFileName(this, tr("Rename File"), m_filename, tr("Plain Text (*.txt);;All Files (*)"));
	if (!filename.isEmpty()) {
		if (m_auto_append && !filename.endsWith(".txt")) {
			filename.append(".txt");
		}
		QFile::remove(filename);
		QFile::rename(m_filename, filename);
		m_filename = filename;
		updateSaveLocation();
		return true;
	} else {
		return false;
	}
}

/*****************************************************************************/

void Document::checkSpelling() {
	SpellChecker::checkDocument(m_text);
}

/*****************************************************************************/

void Document::find() {
	m_find_dialog->show();
	m_find_dialog->raise();
}

/*****************************************************************************/

void Document::print() {
	QPrinter printer;
	printer.setPageSize(QPrinter::Letter);
	printer.setPageMargins(0.5, 0.5, 0.5, 0.5, QPrinter::Inch);
	QPrintDialog dialog(&printer, this);
	if (dialog.exec() == QDialog::Accepted) {
		m_text->print(&printer);
	}
}

/*****************************************************************************/

void Document::loadTheme(const Theme& theme) {
	m_text->document()->blockSignals(true);

	// Update colors
	QPalette p = m_text->palette();
	QColor color = theme.foregroundColor();
	color.setAlpha(theme.foregroundOpacity() * 2.55f);
	p.setColor(QPalette::Base, color);
	p.setColor(QPalette::Text, theme.textColor());
	p.setColor(QPalette::Highlight, theme.textColor());
	p.setColor(QPalette::HighlightedText, theme.foregroundColor());
	m_text->setPalette(p);
	m_highlighter->setMisspelledColor(theme.misspelledColor());

	// Update text
	QFont font = theme.textFont();
	font.setStyleStrategy(m_text->font().styleStrategy());
	m_text->setFont(font);
	m_text->setFixedWidth(theme.foregroundWidth());
	m_text->setCursorWidth(!m_block_cursor ? 1 : m_text->fontMetrics().averageCharWidth());

	m_text->document()->blockSignals(false);
}

/*****************************************************************************/

void Document::loadPreferences(const Preferences& preferences) {
	if (preferences.alwaysCenter()) {
		connect(m_text, SIGNAL(textChanged()), m_text, SLOT(centerCursor()));
	} else {
		disconnect(m_text, SIGNAL(textChanged()), m_text, SLOT(centerCursor()));
	}

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

	m_auto_append = preferences.autoAppend();
	m_block_cursor = preferences.blockCursor();
	m_text->setCursorWidth(!m_block_cursor ? 1 : m_text->fontMetrics().averageCharWidth());
	QFont font = m_text->font();
	font.setStyleStrategy(preferences.smoothFonts() ? QFont::PreferAntialias : QFont::NoAntialias);
	m_text->setFont(font);

	m_highlighter->setEnabled(preferences.highlightMisspelled());
}

/*****************************************************************************/

void Document::setMargin(int margin) {
	m_margin = margin;
	m_layout->setColumnMinimumWidth(0, m_margin);
	m_layout->setColumnMinimumWidth(2, m_margin);
	m_layout->setRowMinimumHeight(0, m_margin);
	m_layout->setRowMinimumHeight(2, m_margin);
}

/*****************************************************************************/

bool Document::eventFilter(QObject* watched, QEvent* event) {
	if (event->type() == QEvent::MouseMove) {
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
	}
	return QWidget::eventFilter(watched, event);
}

/*****************************************************************************/

void Document::mouseMoveEvent(QMouseEvent* event) {
	m_text->viewport()->setCursor(Qt::IBeamCursor);
	unsetCursor();
	m_hide_timer->start();

	const QPoint& point = mapFromGlobal(event->globalPos());
	bool header_visible = point.y() <= m_margin;
	bool footer_visible = point.y() >= (height() - m_margin);
	m_scrollbar->setVisible(point.x() >= (width() - m_margin) && !header_visible && !footer_visible);
	emit headerVisible(header_visible);
	emit footerVisible(footer_visible);

	return QWidget::mouseMoveEvent(event);
}

/*****************************************************************************/

void Document::wheelEvent(QWheelEvent* event) {
	int delta = event->delta();
	if ( (delta > 0 && m_scrollbar->value() > m_scrollbar->minimum()) || (delta < 0 && m_scrollbar->value() < m_scrollbar->maximum()) ) {
		QApplication::sendEvent(m_scrollbar, event);
	}
	return QWidget::wheelEvent(event);
}

/*****************************************************************************/

void Document::hideMouse() {
	QWidget* widget = QApplication::widgetAt(QCursor::pos());
	if (widget == m_text->viewport() || widget == this) {
		m_text->viewport()->setCursor(Qt::BlankCursor);
		setCursor(Qt::BlankCursor);
	}
}

/*****************************************************************************/

void Document::updateWordCount(int position, int removed, int added) {
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
			static_cast<BlockStats*>(i.userData())->update(i.text());
		}
	}

	int words = m_wordcount;
	calculateWordCount();
	m_current_wordcount += (m_wordcount - words);

	int msecs = m_time.restart();
	if (msecs < 30000) {
		m_current_time += msecs;
	}

	emit changed();
}

/*****************************************************************************/

void Document::calculateWordCount() {
	m_character_count = 0;
	m_paragraph_count = 0;
	m_space_count = 0;
	m_wordcount = 0;
	for (QTextBlock i = m_text->document()->begin(); i != m_text->document()->end(); i = i.next()) {
		if (!i.userData()) {
			i.setUserData(new BlockStats(i.text()));
		}
		BlockStats* stats = static_cast<BlockStats*>(i.userData());
		m_character_count += stats->characterCount();
		m_paragraph_count += !stats->isEmpty();
		m_space_count += stats->spaceCount();
		m_wordcount += stats->wordCount();
	}
	if (!m_accurate_wordcount) {
		m_wordcount = std::ceil(m_character_count / 6.0f);
	}

	float amount = 0;
	switch (m_page_type) {
	case 1:
		amount = m_paragraph_count;
		break;
	case 2:
		amount = m_wordcount;
		break;
	default:
		amount = m_character_count;
		break;
	}
	m_page_count = std::ceil(amount / m_page_amount);
}

/*****************************************************************************/

void Document::updateSaveLocation() {
	QString path = QFileInfo(m_filename).canonicalPath();
	QSettings().setValue("Save/Location", path);
	QDir::setCurrent(path);
}

/*****************************************************************************/
