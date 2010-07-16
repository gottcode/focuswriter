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

#include "stack.h"

#include "alert_layer.h"
#include "document.h"
#include "find_dialog.h"
#include "theme.h"

#include <QGridLayout>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QStackedLayout>
#include <QTextCursor>
#include <QThread>
#include <QTimer>

/*****************************************************************************/

namespace {
	class BackgroundLoader : public QThread {
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
		QImage m_source;
		QString m_source_path;
		QImage m_image;
		QMutex m_image_mutex;
	} background_loader;

	void BackgroundLoader::create(int position, const QString& image_path, QWidget* widget) {
		File file = { position, image_path, widget->rect(), widget->palette() };

		m_file_mutex.lock();
		m_files.append(file);
		m_file_mutex.unlock();

		if (!isRunning()) {
			start();
		}
	}

	QPixmap BackgroundLoader::pixmap() {
		QMutexLocker locker(&m_image_mutex);
		return QPixmap::fromImage(m_image, Qt::AutoColor | Qt::AvoidDither);
	}

	void BackgroundLoader::reset() {
		QMutexLocker locker(&m_image_mutex);
		m_image = QImage();
	}

	void BackgroundLoader::run() {
		m_file_mutex.lock();
		do {
			File file = m_files.takeLast();
			m_files.clear();
			m_file_mutex.unlock();

			QSize size = file.rect.size();
			QImage scaled = QImage(size, QImage::Format_RGB32);
			if (m_source_path != file.image_path) {
				m_source_path = file.image_path;
				m_source.load(m_source_path);
			}

			QImage background;
			switch (file.position) {
			case 2:
				background = m_source;
				break;
			case 3:
				background = m_source.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				break;
			case 4:
				background = m_source.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				break;
			case 5:
				background = m_source.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
				break;
			default:
				break;
			}

			QPainter painter(&scaled);
			painter.fillRect(file.rect, file.palette.brush(QPalette::Window));
			if (file.position > 1) {
				painter.drawImage(file.rect.center() - background.rect().center(), background);
			}

			m_image_mutex.lock();
			m_image = scaled;
			m_image_mutex.unlock();

			m_file_mutex.lock();
		} while (!m_files.isEmpty());
		m_file_mutex.unlock();
	}
}

/*****************************************************************************/

Stack::Stack(QWidget* parent)
: QWidget(parent),
  m_current_document(0),
  m_background_position(0),
  m_footer_margin(0),
  m_header_margin(0),
  m_footer_visible(0),
  m_header_visible(0) {
	m_contents = new QWidget(this);

	m_documents_layout = new QStackedLayout(m_contents);
	m_documents_layout->setStackingMode(QStackedLayout::StackAll);

	m_alerts = new AlertLayer(this);

	m_find_dialog = new FindDialog(this);
	connect(m_find_dialog, SIGNAL(findNextAvailable(bool)), this, SIGNAL(findNextAvailable(bool)));

	m_layout = new QGridLayout(this);
	m_layout->setMargin(0);
	m_layout->setRowStretch(1, 1);
	m_layout->setColumnStretch(1, 2);
	m_layout->setColumnStretch(2, 1);
	m_layout->setRowMinimumHeight(0, 6);
	m_layout->setRowMinimumHeight(3, 6);
	m_layout->setColumnMinimumWidth(0, 6);
	m_layout->setColumnMinimumWidth(3, 6);
	m_layout->addWidget(m_contents, 0, 0, 4, 4);
	m_layout->addWidget(m_alerts, 2, 2);

	m_resize_timer = new QTimer(this);
	m_resize_timer->setInterval(50);
	m_resize_timer->setSingleShot(true);
	connect(m_resize_timer, SIGNAL(timeout()), this, SLOT(updateBackground()));
	connect(&background_loader, SIGNAL(finished()), this, SLOT(updateBackground()));
}

/*****************************************************************************/

Stack::~Stack() {
	background_loader.wait();
}

/*****************************************************************************/

void Stack::addDocument(Document* document) {
	connect(document, SIGNAL(changedName()), this, SIGNAL(updateFormatActions()));
	connect(document, SIGNAL(footerVisible(bool)), this, SLOT(setFooterVisible(bool)));
	connect(document, SIGNAL(headerVisible(bool)), this, SLOT(setHeaderVisible(bool)));
	connect(document->text(), SIGNAL(copyAvailable(bool)), this, SIGNAL(copyAvailable(bool)));
	connect(document->text(), SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
	connect(document->text(), SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
	connect(document->text(), SIGNAL(currentCharFormatChanged(const QTextCharFormat&)), this, SIGNAL(updateFormatActions()));

	document->setBackground(m_background);
	m_documents.append(document);
	m_documents_layout->addWidget(document);

	emit documentAdded(document);
	emit updateFormatActions();
}

/*****************************************************************************/

void Stack::moveDocument(int from, int to) {
	m_documents.move(from, to);
}

/*****************************************************************************/

void Stack::removeDocument(int index) {
	Document* document = m_documents.takeAt(index);
	m_documents_layout->removeWidget(document);
	emit documentRemoved(document);
	document->deleteLater();
}

/*****************************************************************************/

void Stack::setCurrentDocument(int index) {
	m_current_document = m_documents[index];
	m_documents_layout->setCurrentWidget(m_current_document);

	emit copyAvailable(!m_current_document->text()->textCursor().selectedText().isEmpty());
	emit redoAvailable(m_current_document->text()->document()->isRedoAvailable());
	emit undoAvailable(m_current_document->text()->document()->isUndoAvailable());
	emit updateFormatActions();
}

/*****************************************************************************/

void Stack::setMargins(int footer, int header) {
	m_footer_margin = footer;
	m_header_margin = header;
	m_footer_visible = (m_footer_visible != 0) ? -m_footer_margin : 0;
	m_header_visible = (m_header_visible != 0) ? m_header_margin : 0;

	int margin = qMax(m_footer_margin, m_header_margin) + 6;
	m_layout->setRowMinimumHeight(0, margin);
	m_layout->setRowMinimumHeight(3, margin);
	m_layout->setColumnMinimumWidth(0, margin);
	m_layout->setColumnMinimumWidth(3, margin);

	updateMask();
}

/*****************************************************************************/

void Stack::alignCenter() {
	m_current_document->text()->setAlignment(Qt::AlignCenter);
}

/*****************************************************************************/

void Stack::alignJustify() {
	m_current_document->text()->setAlignment(Qt::AlignJustify);
}

/*****************************************************************************/

void Stack::alignLeft() {
	m_current_document->text()->setAlignment(Qt::AlignLeft);
}

/*****************************************************************************/

void Stack::alignRight() {
	m_current_document->text()->setAlignment(Qt::AlignRight);
}

/*****************************************************************************/

void Stack::autoSave() {
	foreach (Document* document, m_documents) {
		if (document->text()->document()->isModified() && !document->filename().isEmpty()) {
			document->save();
		}
	}
}

/*****************************************************************************/

void Stack::checkSpelling() {
	m_current_document->checkSpelling();
}

/*****************************************************************************/

void Stack::cut() {
	m_current_document->text()->cut();
}

/*****************************************************************************/

void Stack::copy() {
	m_current_document->text()->copy();
}

/*****************************************************************************/

void Stack::decreaseIndent() {
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat format = cursor.blockFormat();
	format.setIndent(format.indent() - 1);
	cursor.setBlockFormat(format);
	emit updateFormatActions();
}

/*****************************************************************************/

void Stack::find() {
	m_find_dialog->showFindMode();
}

/*****************************************************************************/

void Stack::findNext() {
	m_find_dialog->findNext();
}

/*****************************************************************************/

void Stack::findPrevious() {
	m_find_dialog->findPrevious();
}

/*****************************************************************************/

void Stack::increaseIndent() {
	QTextCursor cursor = m_current_document->text()->textCursor();
	QTextBlockFormat format = cursor.blockFormat();
	format.setIndent(format.indent() + 1);
	cursor.setBlockFormat(format);
	emit updateFormatActions();
}

/*****************************************************************************/

void Stack::makePlainText() {
	if (QMessageBox::warning(window(), tr("Question"), tr("Remove all formatting from the current file?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}
	m_current_document->setRichText(false);
}

/*****************************************************************************/

void Stack::makeRichText() {
	m_current_document->setRichText(true);
}

/*****************************************************************************/

void Stack::paste() {
	m_current_document->text()->paste();
}

/*****************************************************************************/

void Stack::print() {
	m_current_document->print();
}

/*****************************************************************************/

void Stack::redo() {
	m_current_document->text()->redo();
}

/*****************************************************************************/

void Stack::replace() {
	m_find_dialog->showReplaceMode();
}

/*****************************************************************************/

void Stack::save() {
	m_current_document->save();
}

/*****************************************************************************/

void Stack::saveAs() {
	m_current_document->saveAs();
}

/*****************************************************************************/

void Stack::selectAll() {
	m_current_document->text()->selectAll();
}

/*****************************************************************************/

void Stack::setFontBold(bool bold) {
	m_current_document->text()->setFontWeight(bold ? QFont::Bold : QFont::Normal);
}

/*****************************************************************************/

void Stack::setFontItalic(bool italic) {
	m_current_document->text()->setFontItalic(italic);
}

/*****************************************************************************/

void Stack::setFontStrikeOut(bool strikeout) {
	QTextCharFormat format;
	format.setFontStrikeOut(strikeout);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

/*****************************************************************************/

void Stack::setFontUnderline(bool underline) {
	m_current_document->text()->setFontUnderline(underline);
}

/*****************************************************************************/

void Stack::setFontSuperScript(bool super) {
	QTextCharFormat format;
	format.setVerticalAlignment(super ? QTextCharFormat::AlignSuperScript : QTextCharFormat::AlignNormal);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

/*****************************************************************************/

void Stack::setFontSubScript(bool sub) {
	QTextCharFormat format;
	format.setVerticalAlignment(sub ? QTextCharFormat::AlignSubScript : QTextCharFormat::AlignNormal);
	m_current_document->text()->mergeCurrentCharFormat(format);
}

/*****************************************************************************/

void Stack::themeSelected(const Theme& theme) {
	m_background_position = theme.backgroundType();
	m_background_path = theme.backgroundImage();

	QPalette p = palette();
	if (m_background_position != 1) {
		p.setColor(QPalette::Window, theme.backgroundColor().rgb());
	} else {
		p.setBrush(QPalette::Window, QImage(m_background_path));
	}
	setPalette(p);

	background_loader.reset();
	updateBackground();

	foreach (Document* document, m_documents) {
		document->loadTheme(theme);
	}
}

/*****************************************************************************/

void Stack::undo() {
	m_current_document->text()->undo();
}

/*****************************************************************************/

void Stack::setFooterVisible(bool visible) {
	m_footer_visible = visible * -m_footer_margin;
	updateMask();
}

/*****************************************************************************/

void Stack::setHeaderVisible(bool visible) {
	m_header_visible = visible * m_header_margin;
	updateMask();
}

/*****************************************************************************/

void Stack::resizeEvent(QResizeEvent* event) {
	m_background = QPixmap();
	updateDocumentBackgrounds();
	updateMask();
	m_resize_timer->start();
	QWidget::resizeEvent(event);
}

/*****************************************************************************/

void Stack::updateBackground() {
	m_background = background_loader.pixmap();
	if ((m_background.isNull() || m_background.size() != size()) && isVisible()) {
		m_background = QPixmap();
		background_loader.create(m_background_position, m_background_path, this);
	}
	updateDocumentBackgrounds();
}

/*****************************************************************************/

void Stack::updateDocumentBackgrounds() {
	foreach (Document* document, m_documents) {
		document->setBackground(m_background);
	}
}

/*****************************************************************************/

void Stack::updateMask() {
	setMask(rect().adjusted(0, m_header_visible, 0, m_footer_visible));
	m_contents->setMask(mask());
	foreach (Document* document, m_documents) {
		document->setMask(mask());
	}
	update();
}

/*****************************************************************************/
