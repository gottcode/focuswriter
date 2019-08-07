/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "theme.h"
class AlertLayer;
class Document;
class FindDialog;
class SceneList;
class SymbolsDialog;
class ThemeRenderer;

#include <QWidget>
class QActionGroup;
class QGridLayout;
class QMenu;
class QPrinter;
class QStackedWidget;

class Stack : public QWidget
{
	Q_OBJECT

public:
	Stack(QWidget* parent = 0);
	~Stack();

	void addDocument(Document* document);

	AlertLayer* alerts() const;
	QMenu* menu() const;
	SymbolsDialog* symbols() const;

	int count() const;
	Document* currentDocument() const;
	int currentIndex() const;
	Document* document(int index) const;

	void moveDocument(int from, int to);
	void removeDocument(int index);
	void updateDocument(int index);
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
	void documentSelected(int index);
	void findNextAvailable(bool available);
	void updateFormatActions();
	void updateFormatAlignmentActions();

public slots:
	void alignCenter();
	void alignJustify();
	void alignLeft();
	void alignRight();
	void autoCache();
	void checkSpelling();
	void cut();
	void copy();
	void decreaseIndent();
	void find();
	void findNext();
	void findPrevious();
	void increaseIndent();
	void paste();
	void pasteUnformatted();
	void pageSetup();
	void print();
	void redo();
	void reload();
	void replace();
	void save();
	void saveAs();
	void selectAll();
	void selectScene();
	void setFocusMode(QAction* action);
	void setBlockHeading(int heading);
	void setFontBold(bool bold);
	void setFontItalic(bool italic);
	void setFontStrikeOut(bool strikeout);
	void setFontUnderline(bool underline);
	void setFontSuperScript(bool super);
	void setFontSubScript(bool sub);
	void setTextDirectionLTR();
	void setTextDirectionRTL();
	void showSymbols();
	void themeSelected(const Theme& theme);
	void undo();
	void updateSmartQuotes();
	void updateSmartQuotesSelection();
	void setFooterVisible(bool visible);
	void setHeaderVisible(bool visible);
	void setScenesVisible(bool visible);
	void showHeader();

protected:
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private slots:
	void actionTriggered(QAction* action);
	void insertSymbol(const QString& text);
	void updateBackground();
	void updateBackground(const QImage& image, const QRect& foreground);
	void updateMargin();
	void updateMask();
	void updateMenuIndexes();

private:
	void initPrinter();

private:
	AlertLayer* m_alerts;
	SceneList* m_scenes;
	QMenu* m_menu;
	QActionGroup* m_menu_group;
	QGridLayout* m_layout;
	FindDialog* m_find_dialog;
	SymbolsDialog* m_symbols_dialog;
	QPrinter* m_printer;

	QStackedWidget* m_contents;
	QList<Document*> m_documents;
	QList<QAction*> m_document_actions;
	Document* m_current_document;

	ThemeRenderer* m_theme_renderer;
	QPixmap m_background;
	QSize m_foreground_size;
	Theme m_theme;
	QTimer* m_resize_timer;

	int m_footer_margin;
	int m_header_margin;
	int m_footer_visible;
	int m_header_visible;
};

inline AlertLayer* Stack::alerts() const {
	return m_alerts;
}

inline QMenu* Stack::menu() const {
	return m_menu;
}

inline SymbolsDialog* Stack::symbols() const {
	return m_symbols_dialog;
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
