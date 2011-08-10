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

#ifndef STACK_H
#define STACK_H

class AlertLayer;
class Document;
class FindDialog;
class LoadScreen;
class Theme;

#include <QWidget>
class QGridLayout;
class QStackedWidget;

class Stack : public QWidget
{
	Q_OBJECT

public:
	Stack(QWidget* parent = 0);
	~Stack();

	void addDocument(Document* document);

	AlertLayer* alerts() const;
	LoadScreen* loadScreen() const;

	int count() const;
	Document* currentDocument() const;
	int currentIndex() const;
	Document* document(int index) const;

	void moveDocument(int from, int to);
	void removeDocument(int index);
	void setCurrentDocument(int index);
	void setMargins(int footer, int header);
	void waitForThemeBackground();

	virtual bool eventFilter(QObject* watched, QEvent* event);

signals:
	void copyAvailable(bool);
	void redoAvailable(bool);
	void undoAvailable(bool);
	void footerVisible(bool);
	void headerVisible(bool);
	void documentAdded(Document* document);
	void documentRemoved(Document* document);
	void findNextAvailable(bool available);
	void formattingEnabled(bool enabled);
	void updateFormatActions();
	void updateFormatAlignmentActions();

public slots:
	void alignCenter();
	void alignJustify();
	void alignLeft();
	void alignRight();
	void autoCache();
	void autoSave();
	void checkSpelling();
    void focusMode_(int level);
    void focusMode1();
    void focusMode2();
    void focusMode3();
    void focusMode0();
	void cut();
	void copy();
	void decreaseIndent();
	void find();
	void findNext();
	void findPrevious();
	void increaseIndent();
	void makePlainText();
	void makeRichText();
	void paste();
	void print();
	void redo();
	void replace();
	void save();
	void saveAs();
	void selectAll();
	void setFontBold(bool bold);
	void setFontItalic(bool italic);
	void setFontStrikeOut(bool strikeout);
	void setFontUnderline(bool underline);
	void setFontSuperScript(bool super);
	void setFontSubScript(bool sub);
	void setTextDirectionLTR();
	void setTextDirectionRTL();
	void themeSelected(const Theme& theme);
	void undo();
	void updateSmartQuotes();
	void updateSmartQuotesSelection();
	void setFooterVisible(bool visible);
	void setHeaderVisible(bool visible = true);
	void showHeader();

protected:
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private slots:
	void updateBackground();
	void updateMask();
	void updateMapping();

private:
	LoadScreen* m_load_screen;
	AlertLayer* m_alerts;
	QGridLayout* m_layout;
	FindDialog* m_find_dialog;

	QStackedWidget* m_contents;
	QList<Document*> m_documents;
	Document* m_current_document;

	QPixmap m_background;
	int m_background_position;
	QString m_background_path;
	QTimer* m_resize_timer;

	int m_margin;
	int m_footer_margin;
	int m_header_margin;
	int m_footer_visible;
	int m_header_visible;
};

inline AlertLayer* Stack::alerts() const {
	return m_alerts;
}

inline LoadScreen* Stack::loadScreen() const {
	return m_load_screen;
}

inline int Stack::count() const {
	return m_documents.count();
}

inline Document* Stack::currentDocument() const {
	return m_current_document;
}

inline int Stack::currentIndex() const {
	return m_documents.indexOf(m_current_document);
}

inline Document* Stack::document(int index) const {
	return m_documents[index];
}

#endif
