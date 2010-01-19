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

#ifndef STACK_H
#define STACK_H

#include <QWidget>
class QStackedLayout;
class Document;
class Theme;

class Stack : public QWidget {
	Q_OBJECT
public:
	Stack(QWidget* parent = 0);
	~Stack();

	void addDocument(Document* document);

	int count() const {
		return m_documents.count();
	}

	Document* currentDocument() const {
		return m_current_document;
	}

	int currentIndex() const {
		return m_documents.indexOf(m_current_document);
	}

	Document* document(int index) const {
		return m_documents[index];
	}

	void moveDocument(int from, int to);

	void removeDocument(int index);

	void setCurrentDocument(int index);

signals:
	void copyAvailable(bool);
	void redoAvailable(bool);
	void undoAvailable(bool);

public slots:
	void autoSave();
	void checkSpelling();
	void cut();
	void copy();
	void find();
	void paste();
	void print();
	void redo();
	void save();
	void saveAs();
	void selectAll();
	void themeSelected(const Theme& theme);
	void undo();

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private slots:
	void updateBackground();

private:
	QStackedLayout* m_layout;

	QList<Document*> m_documents;
	Document* m_current_document;

	QPixmap m_background;
	int m_background_position;
	QString m_background_path;
	QTimer* m_resize_timer;
};

#endif
