/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "window.h"

#include "document.h"
#include "preferences.h"
#include "preferences_dialog.h"
#include "stack.h"
#include "theme.h"
#include "theme_manager.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDate>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSettings>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>

/*****************************************************************************/

Window::Window()
: m_toolbar(0),
  m_margin(0),
  m_fullscreen(true),
  m_auto_save(true),
  m_goal_type(0),
  m_time_goal(0),
  m_wordcount_goal(0),
  m_current_time(0),
  m_current_wordcount(0) {
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowIcon(QIcon(":/focuswriter.png"));

	// Add contents
	QWidget* contents = new QWidget(this);
	setCentralWidget(contents);
	m_documents = new Stack(contents);

	// Set up menubar and toolbar
	m_header = new QWidget(this);
	m_header->setAutoFillBackground(true);
	m_header->setVisible(false);
	initMenuBar();
	initToolBar();

	// Lay out header
	QVBoxLayout* header_layout = new QVBoxLayout(m_header);
	header_layout->setMargin(0);
	header_layout->setSpacing(0);
#ifndef Q_OS_MAC
	header_layout->addWidget(m_menubar);
#endif
	header_layout->addWidget(m_toolbar);

	// Set up details
	m_footer = new QWidget(this);
	m_footer->setAutoFillBackground(true);
	m_footer->setVisible(false);
	QWidget* details = new QWidget(m_footer);
	m_wordcount_label = new QLabel(tr("Words: 0"), details);
	m_page_label = new QLabel(tr("Pages: 0"), details);
	m_paragraph_label = new QLabel(tr("Paragraphs: 0"), details);
	m_character_label = new QLabel(tr("Characters: 0"), details);
	m_progress_label = new QLabel(tr("0% of daily goal"), details);
	m_clock_label = new QLabel(details);
	updateClock();

	// Set up clock
	m_clock_timer = new QTimer(this);
	m_clock_timer->setInterval(60000);
	connect(m_clock_timer, SIGNAL(timeout()), this, SLOT(updateClock()));
	int delay = (60 - QTime::currentTime().second()) * 1000;
	QTimer::singleShot(delay, m_clock_timer, SLOT(start()));
	QTimer::singleShot(delay, this, SLOT(updateClock()));

	// Set up tabs
	m_tabs = new QTabBar(m_footer);
	m_tabs->setShape(QTabBar::RoundedSouth);
	m_tabs->setDocumentMode(true);
	m_tabs->setExpanding(false);
	m_tabs->setMovable(true);
	m_tabs->setTabsClosable(true);
	m_tabs->setUsesScrollButtons(true);
	connect(m_tabs, SIGNAL(currentChanged(int)), this, SLOT(tabClicked(int)));
	connect(m_tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClosed(int)));
	connect(m_tabs, SIGNAL(tabMoved(int, int)), this, SLOT(tabMoved(int, int)));

	// Lay out details
	QHBoxLayout* details_layout = new QHBoxLayout(details);
	details_layout->setSpacing(25);
	details_layout->setMargin(6);
	details_layout->addWidget(m_wordcount_label);
	details_layout->addWidget(m_page_label);
	details_layout->addWidget(m_paragraph_label);
	details_layout->addWidget(m_character_label);
	details_layout->addStretch();
	details_layout->addWidget(m_progress_label);
	details_layout->addStretch();
	details_layout->addWidget(m_clock_label);

	// Lay out footer
	QVBoxLayout* footer_layout = new QVBoxLayout(m_footer);
	footer_layout->setSpacing(0);
	footer_layout->setMargin(0);
	footer_layout->addWidget(details);
	footer_layout->addWidget(m_tabs);

	// Lay out window
	QGridLayout* layout = new QGridLayout(contents);
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->setRowStretch(1, 1);
	layout->addWidget(m_documents, 0, 0, 3, 3);
	layout->addWidget(m_header, 0, 0, 1, 3);
	layout->addWidget(m_footer, 2, 0, 1, 3);

	// Load current daily progress
	QSettings settings;
	if (settings.value("Progress/Date").toDate() != QDate::currentDate()) {
		settings.remove("Progress");
	}
	settings.setValue("Progress/Date", QDate::currentDate().toString(Qt::ISODate));
	m_current_wordcount = settings.value("Progress/Words", 0).toInt();
	m_current_time = settings.value("Progress/Time", 0).toInt();
	updateProgress();

	// Load settings
	Preferences preferences;
	loadPreferences(preferences);
	m_documents->themeSelected(Theme(settings.value("ThemeManager/Theme").toString()));

	// Restore window geometry
	restoreGeometry(settings.value("Window/Geometry").toByteArray());
	m_fullscreen = !settings.value("Window/Fullscreen", true).toBool();
	toggleFullscreen();

	// Open previous documents
	QStringList files = QApplication::arguments();
	files.removeFirst();
	if (files.isEmpty()) {
		files = settings.value("Save/Current").toStringList();
		if (files.isEmpty()) {
			files.append(QString());
		}
	}
	foreach (const QString& file, files) {
		addDocument(file);
	}
	m_tabs->setCurrentIndex(settings.value("Save/Active", 0).toInt());
	updateMargin();
}

/*****************************************************************************/

bool Window::event(QEvent* event) {
	if (event->type() == QEvent::WindowBlocked) {
		hideInterface();
	}
	return QMainWindow::event(event);
}

/*****************************************************************************/

void Window::closeEvent(QCloseEvent* event) {
	QStringList files;
	int index = m_tabs->currentIndex();
	for (int i = 0; i < m_documents->count(); ++i) {
		Document* document = m_documents->document(i);
		QString filename = document->filename();
		if (!filename.isEmpty()) {
			files.append(filename);
		}

		m_tabs->setCurrentIndex(i);
		if (!saveDocument(i)) {
			event->ignore();
			m_tabs->setCurrentIndex(index);
			return;
		}
	}
	m_tabs->setCurrentIndex(index);

	QSettings settings;
	settings.setValue("Save/Current", files);
	settings.setValue("Save/Active", m_tabs->currentIndex());

	event->accept();
	QMainWindow::closeEvent(event);
}

/*****************************************************************************/

void Window::leaveEvent(QEvent* event) {
	if (qApp->activePopupWidget() == 0) {
		hideInterface();
	}
	QMainWindow::leaveEvent(event);
}

/*****************************************************************************/

void Window::resizeEvent(QResizeEvent* event) {
	if (!m_fullscreen) {
		QSettings().setValue("Window/Geometry", saveGeometry());
	}
	QMainWindow::resizeEvent(event);
}

/*****************************************************************************/

void Window::newDocument() {
	addDocument();
	m_actions["Rename"]->setEnabled(false);
}

/*****************************************************************************/

void Window::openDocument() {
	QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), tr("Plain Text (*.txt);;All Files (*)"));
	if (!filename.isEmpty()) {
		addDocument(filename);
	}
}

/*****************************************************************************/

void Window::renameDocument() {
	if (m_documents->currentDocument()->rename()) {
		updateTab(m_documents->currentIndex());
	}
}

/*****************************************************************************/

void Window::saveAllDocuments() {
	int index = m_tabs->currentIndex();
	for (int i = 0; i < m_documents->count(); ++i) {
		Document* document = m_documents->document(i);
		if (!document->filename().isEmpty()) {
			document->save();
		} else {
			m_tabs->setCurrentIndex(i);
			document->saveAs();
		}
	}
	m_tabs->setCurrentIndex(index);
}

/*****************************************************************************/

void Window::closeDocument() {
	int index = m_documents->currentIndex();
	if (!saveDocument(index)) {
		return;
	}
	if (m_documents->count() == 1) {
		newDocument();
	}
	m_documents->removeDocument(index);
	m_tabs->removeTab(index);
}

/*****************************************************************************/

void Window::toggleFullscreen() {
	m_fullscreen = !m_fullscreen;
	QSettings().setValue("Window/Fullscreen", m_fullscreen);

	if (m_fullscreen) {
		showFullScreen();
		m_actions["Fullscreen"]->setText(tr("Leave Fullscreen"));
	} else {
		showNormal();
		m_actions["Fullscreen"]->setText(tr("Fullscreen"));
	}
	QApplication::processEvents();
	raise();
	activateWindow();
}

/*****************************************************************************/

void Window::toggleToolbar(bool visible) {
	m_toolbar->setVisible(visible);
	QSettings().setValue("Toolbar/Shown", visible);
	updateMargin();
}

/*****************************************************************************/

void Window::themeClicked() {
	ThemeManager manager(this);
	connect(&manager, SIGNAL(themeSelected(const Theme&)), m_documents, SLOT(themeSelected(const Theme&)));
	manager.exec();
}

/*****************************************************************************/

void Window::preferencesClicked() {
	Preferences preferences;
	PreferencesDialog dialog(preferences, this);
	if (dialog.exec() == QDialog::Accepted) {
		loadPreferences(preferences);
	}
}

/*****************************************************************************/

void Window::aboutClicked() {
	QMessageBox::about(this, tr("About FocusWriter"), tr(
		"<p><center><big><b>FocusWriter %1</b></big><br/>"
		"A simple fullscreen word processor<br/>"
		"<small>Copyright &copy; 2008-2009 Graeme Gott</small><br/>"
		"<small>Released under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GPL 3</a> license</small></center></p>"
		"<p><center>Includes <a href=\"http://hunspell.sourceforge.net/\">Hunspell</a> 1.2.8 for spell checking<br/>"
		"<small>Used under the <a href=\"http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html\">LGPL 2.1</a> license</small></center></p>"
		"<p><center>Includes icons from the <a href=\"http://www.oxygen-icons.org/\">Oxygen</a> icon theme<br/>"
		"<small>Used under the <a href=\"http://www.gnu.org/licenses/lgpl.html\">LGPL 3</a> license</small></center></p>"
	).arg(QApplication::applicationVersion()));
}

/*****************************************************************************/

void Window::tabClicked(int index) {
	m_documents->setCurrentDocument(index);
	updateDetails();
	updateSave();
	m_documents->currentDocument()->text()->setFocus();
}

/*****************************************************************************/

void Window::tabClosed(int index) {
	m_tabs->setCurrentIndex(index);
	closeDocument();
}

/*****************************************************************************/

void Window::tabMoved(int from, int to) {
	m_documents->moveDocument(from, to);
	m_documents->setCurrentDocument(m_tabs->currentIndex());
}

/*****************************************************************************/

void Window::updateClock() {
	m_clock_label->setText(QTime::currentTime().toString(Qt::DefaultLocaleShortDate));
}

/*****************************************************************************/

void Window::updateDetails() {
	Document* document = m_documents->currentDocument();
	m_character_label->setText(tr("Characters: %L1 / %L2").arg(document->characterCount()).arg(document->characterAndSpaceCount()));
	m_page_label->setText(tr("Pages: %L1").arg(document->pageCount()));
	m_paragraph_label->setText(tr("Paragraphs: %L1").arg(document->paragraphCount()));
	m_wordcount_label->setText(tr("Words: %L1").arg(document->wordCount()));
}

/*****************************************************************************/

void Window::updateProgress() {
	int progress = 0;
	if (m_goal_type == 1) {
		progress = (m_current_time * 100) / (m_time_goal * 60000);
	} else if (m_goal_type == 2) {
		progress = (m_current_wordcount * 100) / m_wordcount_goal;
	}
	m_progress_label->setText(tr("%1% of daily goal").arg(progress));
}

/*****************************************************************************/

void Window::updateSave() {
	m_actions["Save"]->setEnabled(m_documents->currentDocument()->text()->document()->isModified());
	m_actions["Rename"]->setDisabled(m_documents->currentDocument()->filename().isEmpty());
	for (int i = 0; i < m_documents->count(); ++i) {
		updateTab(i);
	}
}

/*****************************************************************************/

void Window::addDocument(const QString& filename) {
	Document* document = new Document(filename, m_current_wordcount, m_current_time, size(), m_margin, this);
	connect(document, SIGNAL(changed()), this, SLOT(updateDetails()));
	connect(document, SIGNAL(changed()), this, SLOT(updateProgress()));
	connect(document, SIGNAL(footerVisible(bool)), m_footer, SLOT(setVisible(bool)));
	connect(document, SIGNAL(headerVisible(bool)), m_header, SLOT(setVisible(bool)));
	connect(document->text(), SIGNAL(modificationChanged(bool)), this, SLOT(updateSave()));

	m_documents->addDocument(document);

	int index = m_tabs->addTab(tr("Untitled"));
	updateTab(index);
	m_tabs->setCurrentIndex(index);

	document->text()->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
	document->text()->centerCursor();
}

/*****************************************************************************/

bool Window::saveDocument(int index) {
	Document* document = m_documents->document(index);
	if (!document->text()->document()->isModified()) {
		return true;
	}

	// Auto-save document
	if (m_auto_save && document->text()->document()->isModified() && !document->filename().isEmpty()) {
		document->save();
		return true;
	}

	// Prompt about saving changes
	switch (QMessageBox::question(this, tr("Question"), tr("Save changes?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel)) {
	case QMessageBox::Save:
		return document->save();
	case QMessageBox::Discard:
		document->text()->document()->setModified(false);
		return true;
	case QMessageBox::Cancel:
	default:
		return false;
	}
}

/*****************************************************************************/

void Window::loadPreferences(const Preferences& preferences) {
	m_auto_save = preferences.autoSave();
	if (m_auto_save) {
		connect(m_clock_timer, SIGNAL(timeout()), m_documents, SLOT(autoSave()));
	} else {
		disconnect(m_clock_timer, SIGNAL(timeout()), m_documents, SLOT(autoSave()));
	}

	m_character_label->setVisible(preferences.showCharacters());
	m_page_label->setVisible(preferences.showPages());
	m_paragraph_label->setVisible(preferences.showParagraphs());
	m_wordcount_label->setVisible(preferences.showWords());
	m_progress_label->setVisible(preferences.goalType() != 0);

	m_goal_type = preferences.goalType();
	m_wordcount_goal = preferences.goalWords();
	m_time_goal = preferences.goalMinutes();
	updateProgress();

	m_toolbar->clear();
	m_toolbar->hide();
	m_toolbar->setToolButtonStyle(Qt::ToolButtonStyle(preferences.toolbarStyle()));
	QStringList actions = preferences.toolbarActions();
	foreach (const QString action, actions) {
		if (action == "|") {
			m_toolbar->addSeparator();
		} else if (!action.startsWith("^")) {
			m_toolbar->addAction(m_actions.value(action));
		}
	}
	m_toolbar->setVisible(QSettings().value("Toolbar/Shown", true).toBool());
	updateMargin();

	for (int i = 0; i < m_documents->count(); ++i) {
		m_documents->document(i)->loadPreferences(preferences);
	}
	if (m_documents->count() > 0) {
		updateDetails();
	}
}

/*****************************************************************************/

void Window::hideInterface() {
	m_header->hide();
	m_footer->hide();
	for (int i = 0; i < m_documents->count(); ++i) {
		m_documents->document(i)->setScrollBarVisible(false);
	}
}

/*****************************************************************************/

void Window::updateMargin() {
	m_header->show();
	m_footer->show();
	m_margin = qMax(m_header->sizeHint().height(), m_footer->sizeHint().height());
	m_header->hide();
	m_footer->hide();

	for (int i = 0; i < m_documents->count(); ++i) {
		m_documents->document(i)->setMargin(m_margin);
	}
}

/*****************************************************************************/

void Window::updateTab(int index) {
	Document* document = m_documents->document(index);
	QString filename = document->filename();
	if (filename.isEmpty()) {
		filename = tr("(Untitled %1)").arg(document->index());
	}
	bool modified = document->text()->document()->isModified();
	m_tabs->setTabText(index, QFileInfo(filename).fileName() + (modified ? "*" : ""));
	m_tabs->setTabToolTip(index, QDir::toNativeSeparators(filename));
	if (document == m_documents->currentDocument()) {
		setWindowFilePath(filename);
		setWindowModified(modified);
	}
}

/*****************************************************************************/

void Window::initMenuBar() {
	m_menubar = menuBar();

	// Create file menu
	QMenu* file_menu = m_menubar->addMenu(tr("&File"));
	m_actions["New"] = file_menu->addAction(tr("&New"), this, SLOT(newDocument()), tr("Ctrl+N"));
	m_actions["Open"] = file_menu->addAction(tr("&Open..."), this, SLOT(openDocument()), tr("Ctrl+O"));
	file_menu->addSeparator();
	m_actions["Save"] = file_menu->addAction(tr("&Save"), m_documents, SLOT(save()), tr("Ctrl+S"));
	m_actions["Save"]->setEnabled(false);
	m_actions["SaveAs"] = file_menu->addAction(tr("Save &As..."), m_documents, SLOT(saveAs()));
	m_actions["Rename"] = file_menu->addAction(tr("&Rename..."), this, SLOT(renameDocument()));
	m_actions["Rename"]->setEnabled(false);
	m_actions["SaveAll"] = file_menu->addAction(tr("Save A&ll"), this, SLOT(saveAllDocuments()));
	file_menu->addSeparator();
	m_actions["Print"] = file_menu->addAction(tr("&Print..."), m_documents, SLOT(print()), tr("Ctrl+P"));
	file_menu->addSeparator();
	m_actions["Close"] = file_menu->addAction(tr("&Close"), this, SLOT(closeDocument()), tr("Ctrl+W"));
	m_actions["Quit"] = file_menu->addAction(tr("&Quit"), this, SLOT(close()), tr("Ctrl+Q"));

	// Create edit menu
	QMenu* edit_menu = m_menubar->addMenu(tr("&Edit"));
	m_actions["Undo"] = edit_menu->addAction(tr("&Undo"), m_documents, SLOT(undo()), tr("Ctrl+Z"));
	m_actions["Undo"]->setEnabled(false);
	connect(m_documents, SIGNAL(undoAvailable(bool)), m_actions["Undo"], SLOT(setEnabled(bool)));
	m_actions["Redo"] = edit_menu->addAction(tr("&Redo"), m_documents, SLOT(redo()), tr("Shift+Ctrl+Z"));
	m_actions["Redo"]->setEnabled(false);
	connect(m_documents, SIGNAL(redoAvailable(bool)), m_actions["Redo"], SLOT(setEnabled(bool)));
	edit_menu->addSeparator();
	m_actions["Cut"] = edit_menu->addAction(tr("Cu&t"), m_documents, SLOT(cut()), tr("Ctrl+X"));
	m_actions["Cut"]->setEnabled(false);
	connect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Cut"], SLOT(setEnabled(bool)));
	m_actions["Copy"] = edit_menu->addAction(tr("&Copy"), m_documents, SLOT(copy()), tr("Ctrl+C"));
	m_actions["Copy"]->setEnabled(false);
	connect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Copy"], SLOT(setEnabled(bool)));
	m_actions["Paste"] = edit_menu->addAction(tr("&Paste"), m_documents, SLOT(paste()), tr("Ctrl+V"));
	edit_menu->addSeparator();
	m_actions["SelectAll"] = edit_menu->addAction(tr("Select &All"), m_documents, SLOT(selectAll()), tr("Ctrl+A"));

	// Create tools menu
	QMenu* tools_menu = m_menubar->addMenu(tr("&Tools"));
	m_actions["Find"] = tools_menu->addAction(tr("&Find..."), m_documents, SLOT(find()), tr("Ctrl+F"));
	m_actions["CheckSpelling"] = tools_menu->addAction(tr("&Spelling..."), m_documents, SLOT(checkSpelling()), tr("F7"));

	// Create settings menu
	QMenu* settings_menu = m_menubar->addMenu(tr("&Settings"));
	QAction* action = settings_menu->addAction(tr("Show &Toolbar"), this, SLOT(toggleToolbar(bool)));
	action->setCheckable(true);
	action->setChecked(QSettings().value("Toolbar/Shown", true).toBool());
	settings_menu->addSeparator();
	m_actions["Fullscreen"] = settings_menu->addAction(tr("&Fullscreen"), this, SLOT(toggleFullscreen()), tr("F11"));
#ifdef Q_OS_MAC
	m_actions["Fullscreen"]->setShortcut(tr("Esc"));
#endif
	settings_menu->addSeparator();
	m_actions["Themes"] = settings_menu->addAction(tr("&Themes..."), this, SLOT(themeClicked()));
	m_actions["Preferences"] = settings_menu->addAction(tr("&Preferences..."), this, SLOT(preferencesClicked()));

	// Create help menu
	QMenu* help_menu = m_menubar->addMenu(tr("&Help"));
	m_actions["About"] = help_menu->addAction(tr("&About"), this, SLOT(aboutClicked()));
	m_actions["AboutQt"] = help_menu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));

	// Enable shortcuts when menubar is hidden
	addActions(m_actions.values());
}

/*****************************************************************************/

void Window::initToolBar() {
	m_toolbar = new QToolBar(m_header);
	m_toolbar->setIconSize(QSize(22,22));

	QHashIterator<QString, QAction*> action(m_actions);
	while (action.hasNext()) {
		action.next();
		action.value()->setData(action.key());
	}

	QHash<QString, QString> icons;
	icons["About"] = "help-about";
	icons["CheckSpelling"] = "tools-check-spelling";
	icons["Close"] = "window-close";
	icons["Copy"] = "edit-copy";
	icons["Cut"] = "edit-cut";
	icons["Find"] = "edit-find";
	icons["Fullscreen"] = "view-fullscreen";
	icons["New"] = "document-new";
	icons["Open"] = "document-open";
	icons["Paste"] = "edit-paste";
	icons["Preferences"] = "configure";
	icons["Print"] = "document-print";
	icons["Quit"] = "application-exit";
	icons["Redo"] = "edit-redo";
	icons["Rename"] = "edit-rename";
	icons["Save"] = "document-save";
	icons["SaveAll"] = "document-save-all";
	icons["SaveAs"] = "document-save-as";
	icons["SelectAll"] = "edit-select-all";
	icons["Themes"] = "applications-graphics";
	icons["Undo"] = "edit-undo";
	QHashIterator<QString, QString> i(icons);
	while (i.hasNext()) {
		i.next();
		QIcon icon(QString(":/oxygen/22x22/%1.png").arg(i.value()));
		icon.addFile(QString(":/oxygen/16x16/%1.png").arg(i.value()));
		m_actions[i.key()]->setIcon(icon);
	}
	m_actions["AboutQt"]->setIcon(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"));
}

/*****************************************************************************/
