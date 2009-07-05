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

#include "stack.h"

#include "document.h"
#include "theme.h"

#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
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
: QStackedWidget(parent),
  m_current_document(0),
  m_background_position(0) {
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
	connect(document->text(), SIGNAL(copyAvailable(bool)), this, SIGNAL(copyAvailable(bool)));
	connect(document->text(), SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
	connect(document->text(), SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));

	m_documents.append(document);
	addWidget(document);
}

/*****************************************************************************/

void Stack::moveDocument(int from, int to) {
	m_documents.move(from, to);
}

/*****************************************************************************/

void Stack::removeDocument(int index) {
	Document* document = m_documents.takeAt(index);
	removeWidget(document);
	delete document;
}

/*****************************************************************************/

void Stack::setCurrentDocument(int index) {
	m_current_document = m_documents[index];
	setCurrentWidget(m_current_document);

	emit copyAvailable(!m_current_document->text()->textCursor().selectedText().isEmpty());
	emit redoAvailable(m_current_document->text()->document()->isRedoAvailable());
	emit undoAvailable(m_current_document->text()->document()->isUndoAvailable());
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

void Stack::find() {
	m_current_document->find();
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

void Stack::paintEvent(QPaintEvent* event) {
	QPainter painter(this);
	if (!m_background.isNull()) {
		painter.drawPixmap(event->rect(), m_background, event->rect());
	}
	painter.end();
	QStackedWidget::paintEvent(event);
}

/*****************************************************************************/

void Stack::resizeEvent(QResizeEvent* event) {
	m_background = QPixmap();
	m_resize_timer->start();
	QStackedWidget::resizeEvent(event);
}

/*****************************************************************************/

void Stack::updateBackground() {
	m_background = background_loader.pixmap();
	if ((m_background.isNull() || m_background.size() != size()) && isVisible()) {
		m_background = QPixmap();
		background_loader.create(m_background_position, m_background_path, this);
	}
	update();
}

/*****************************************************************************/
