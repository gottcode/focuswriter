/***********************************************************************
 *
 * Copyright (C) 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "highlighter.h"
#include "preferences.h"
#include "spell_checker.h"
#include "theme.h"

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

#include <cmath>

/*****************************************************************************/

namespace {
	QList<int> g_untitled_indexes = QList<int>() << 0;

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
}

/*****************************************************************************/

Document::Document(const QString& filename, int& current_wordcount, int& current_time, const QSize& size, int margin, QWidget* parent)
: QWidget(parent),
  m_index(0),
  m_always_center(false),
  m_auto_append(true),
  m_rich_text(false),
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
	m_text = new QTextEdit(this);
	m_text->installEventFilter(this);
	m_text->setTabStopWidth(50);
	m_text->setMinimumHeight(500);
	m_text->setFrameStyle(QFrame::NoFrame);
	m_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_text->viewport()->setMouseTracking(true);
	m_text->viewport()->installEventFilter(this);
	m_highlighter = new Highlighter(m_text);

	// Open file
	bool unknown_rich_text = false;
	if (!filename.isEmpty()) {
		QFile file(filename);
		m_rich_text = (QFileInfo(file).suffix().toLower() == QLatin1String("fwr"));
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));

			bool error = false;
			if (m_rich_text) {
				if (stream.readLine() == "FWR1") {
					m_text->setHtml(stream.readAll());
				} else {
					QMessageBox::information(parent->window(), tr("Sorry"), tr("Unable to read FocusWriter Rich Text file."));
					error = true;
				}
			} else {
				m_text->setPlainText(stream.readAll());
			}
			m_text->document()->setModified(false);
			file.close();

			if (!error) {
				m_filename = QFileInfo(file).canonicalFilePath();
				updateSaveLocation();
			}
		}
	}
	if (m_filename.isEmpty()) {
		findIndex();
		unknown_rich_text = true;
	}

	// Set up scroll bar
	m_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_scrollbar = m_text->verticalScrollBar();
	m_scrollbar->setPalette(QApplication::palette());
	m_scrollbar->setAutoFillBackground(true);
	m_scrollbar->setVisible(false);
	connect(m_scrollbar, SIGNAL(actionTriggered(int)), this, SLOT(scrollBarActionTriggered(int)));
	connect(m_scrollbar, SIGNAL(rangeChanged(int,int)), this, SLOT(scrollBarRangeChanged(int,int)));

	// Lay out window
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(0);
	m_layout->setMargin(0);
	m_layout->addWidget(m_text, 1, 1);
	m_layout->addWidget(m_scrollbar, 1, 2, Qt::AlignRight);
	setMargin(margin);

	// Load settings
	Preferences preferences;
	if (unknown_rich_text) {
		m_rich_text = preferences.richText();
	}
	m_text->setAcceptRichText(m_rich_text);
	loadPreferences(preferences);
	setAutoFillBackground(true);
	loadTheme(Theme(QSettings().value("ThemeManager/Theme").toString()));

	calculateWordCount();
	connect(m_text->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(updateWordCount(int,int,int)));
	connect(m_text, SIGNAL(textChanged()), this, SLOT(centerCursor()));
}

/*****************************************************************************/

Document::~Document() {
	clearIndex();
}

/*****************************************************************************/

bool Document::save() {
	// Save progress
	QSettings settings;
	settings.setValue("Progress/Words", m_current_wordcount);
	settings.setValue("Progress/Time", m_current_time);

	// Write file to disk
	if (!m_filename.isEmpty()) {
		QFile file(m_filename);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			if (!m_rich_text) {
				stream << m_text->toPlainText();
			} else {
				stream << QLatin1String("FWR1\n"
										"<html>\n"
										"<head>\n"
										"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
										"<style type=\"text/css\">\n"
										"p { margin: 0px; text-indent: 0px; white-space: pre-wrap; -qt-block-indent: 0; }\n"
										"</style>\n"
										"</head>\n"
										"<body>\n");
				for (QTextBlock block = m_text->document()->begin(); block.isValid(); block = block.next()) {

					stream << QLatin1String("<p");
					QTextBlockFormat block_format = block.blockFormat();
					Qt::Alignment align = block_format.alignment();
					if (align & Qt::AlignCenter) {
						stream << QLatin1String(" align=\"center\"");
					} else if (align & Qt::AlignRight) {
						stream << QLatin1String(" align=\"right\"");
					} else if (align & Qt::AlignJustify) {
						stream << QLatin1String(" align=\"justify\"");
					}
					QString style;
					if (block_format.indent() > 0) {
						style += QString(" -qt-block-indent: %1;").arg(block_format.indent());
					}
					if (block.begin() == block.end()) {
						style += QLatin1String(" -qt-paragraph-type: empty;");
					}
					if (!style.isEmpty()) {
						stream << QLatin1String(" style=\"") << style.trimmed() << QLatin1Char('"');
					}
					stream << QLatin1Char('>');

					for (QTextBlock::iterator iter = block.begin(); iter != block.end(); ++iter) {
						QTextFragment fragment = iter.fragment();
						QTextCharFormat char_format = fragment.charFormat();
						QString text = fragment.text();
						text.replace(QLatin1String("&"), QLatin1String("&amp;"));
						text.replace(QLatin1String("<"), QLatin1String("&lt;"));
						text.replace(QLatin1String(">"), QLatin1String("&gt;"));
						QString style;
						if (char_format.fontWeight() == QFont::Bold) {
							style += QLatin1String(" font-weight: bold;");
						}
						if (char_format.fontItalic()) {
							style += QLatin1String(" font-style: italic;");
						}
						if (char_format.fontUnderline()) {
							style += QLatin1String(" text-decoration: underline;");
						}
						if (char_format.fontStrikeOut()) {
							style += QLatin1String(" text-decoration: line-through;");
						}
						if (char_format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
							style += QLatin1String(" vertical-align: super;");
						} else if (char_format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
							style += QLatin1String(" vertical-align: sub;");
						}
						if (!style.isEmpty()) {
							stream << QLatin1String("<span style=\"") << style.trimmed() << QLatin1String("\">") << text << QLatin1String("</span>");
						} else {
							stream << text;
						}
					}

					stream << QLatin1String("</p>\n");
				}
				stream << QLatin1String("</body>\n</html>");
			}
		}
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
	QString filename = QFileDialog::getSaveFileName(this, tr("Save File As"), QString(), fileFilter());
	if (!filename.isEmpty()) {
		QString suffix = fileSuffix();
		if (m_auto_append && !filename.endsWith(suffix)) {
			filename.append(suffix);
		}
		m_filename = filename;
		clearIndex();
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

	QString filename = QFileDialog::getSaveFileName(this, tr("Rename File"), m_filename, fileFilter());
	if (!filename.isEmpty()) {
		QString suffix = fileSuffix();
		if (m_auto_append && !filename.endsWith(suffix)) {
			filename.append(suffix);
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

	centerCursor();
	m_text->document()->blockSignals(false);
}

/*****************************************************************************/

void Document::loadPreferences(const Preferences& preferences) {
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

	m_auto_append = preferences.autoAppend();
	m_block_cursor = preferences.blockCursor();
	m_text->setCursorWidth(!m_block_cursor ? 1 : m_text->fontMetrics().averageCharWidth());
	QFont font = m_text->font();
	font.setStyleStrategy(preferences.smoothFonts() ? QFont::PreferAntialias : QFont::NoAntialias);
	m_text->setFont(font);

	m_highlighter->setEnabled(preferences.highlightMisspelled());
}

/*****************************************************************************/

void Document::setBackground(const QPixmap& background) {
	m_background = background;
	update();
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

void Document::setRichText(bool rich_text) {
	m_rich_text = rich_text;
	m_text->setAcceptRichText(m_rich_text);

	if (!m_filename.isEmpty()) {
		m_filename.clear();
		findIndex();
	}

	m_text->setUndoRedoEnabled(false);
	m_text->document()->setModified(false);
	if (!m_rich_text) {
		QTextCursor cursor(m_text->document());
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
		cursor.setBlockFormat(QTextBlockFormat());
		cursor.setCharFormat(QTextCharFormat());
	}
	m_text->document()->setModified(true);
	m_text->setUndoRedoEnabled(true);
}

/*****************************************************************************/

void Document::setScrollBarVisible(bool visible) {
	m_scrollbar->setVisible(visible);
}

/*****************************************************************************/

bool Document::eventFilter(QObject* watched, QEvent* event) {
	if (event->type() == QEvent::MouseMove) {
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
	} else if (event->type() == QEvent::KeyPress && watched == m_text) {
		int msecs = m_time.restart();
		if (msecs < 30000) {
			m_current_time += msecs;
		}
		emit changed();
	}
	return QWidget::eventFilter(watched, event);
}

/*****************************************************************************/

void Document::centerCursor() {
	QRect cursor = m_text->cursorRect();
	QRect viewport = m_text->viewport()->rect();
	if (m_always_center || !viewport.contains(cursor, true)) {
		QPoint offset = viewport.center() - cursor.center();
		QScrollBar* scrollbar = m_text->verticalScrollBar();
		scrollbar->setValue(scrollbar->value() - offset.y());
	}
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

void Document::paintEvent(QPaintEvent* event) {
	QPainter painter(this);
	if (!m_background.isNull()) {
		painter.drawPixmap(event->rect(), m_background, event->rect());
	}
	painter.end();
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
	if (m_text->viewport()->hasFocus() && (widget == m_text->viewport() || widget == this)) {
		m_text->viewport()->setCursor(Qt::BlankCursor);
		setCursor(Qt::BlankCursor);
	}
}

/*****************************************************************************/

void Document::scrollBarActionTriggered(int action) {
	if (action == QAbstractSlider::SliderToMinimum) {
		m_text->moveCursor(QTextCursor::Start);
	} else if (action == QAbstractSlider::SliderToMaximum) {
		m_text->moveCursor(QTextCursor::End);
	}
}

/*****************************************************************************/

void Document::scrollBarRangeChanged(int, int max) {
	m_scrollbar->blockSignals(true);
	m_scrollbar->setMaximum(max + m_text->viewport()->height());
	m_scrollbar->blockSignals(false);
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
	m_page_count = qMax(1.0f, std::ceil(amount / m_page_amount));
}

/*****************************************************************************/

void Document::clearIndex() {
	if (m_index) {
		g_untitled_indexes.removeAll(m_index);
		m_index = 0;
	}
}

/*****************************************************************************/

void Document::findIndex() {
	m_index = g_untitled_indexes.last() + 1;
	g_untitled_indexes.append(m_index);
}

/*****************************************************************************/

QString Document::fileFilter() const {
	return m_rich_text ? tr("FocusWriter Rich Text (*.fwr)") : tr("Plain Text (*.txt);;All Files (*)");
}

/*****************************************************************************/

QString Document::fileSuffix() const {
	return m_rich_text ? QLatin1String(".fwr") : QLatin1String(".txt");
}

/*****************************************************************************/

void Document::updateSaveLocation() {
	QString path = QFileInfo(m_filename).canonicalPath();
	QSettings().setValue("Save/Location", path);
	QDir::setCurrent(path);
}

/*****************************************************************************/
