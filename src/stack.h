/*
	SPDX-FileCopyrightText: 2009-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_STACK_H
#define FOCUSWRITER_STACK_H

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
	explicit Stack(QWidget* parent = nullptr);
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

	bool eventFilter(QObject* watched, QEvent* event) override;

Q_SIGNALS:
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

public Q_SLOTS:
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
	void mouseMoveEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
	void actionTriggered(const QAction* action);
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

inline AlertLayer* Stack::alerts() const
{
	return m_alerts;
}

inline QMenu* Stack::menu() const
{
	return m_menu;
}

inline SymbolsDialog* Stack::symbols() const
{
	return m_symbols_dialog;
}

inline int Stack::count() const
{
	return m_documents.count();
}

inline Document* Stack::currentDocument() const
{
	return m_current_document;
}

inline int Stack::currentIndex() const
{
	return m_documents.indexOf(m_current_document);
}

inline Document* Stack::document(int index) const
{
	return m_documents[index];
}

#endif // FOCUSWRITER_STACK_H
