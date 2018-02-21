/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 Graeme Gott <graeme@gottcode.org>
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

#include "action_manager.h"
#include "alert.h"
#include "alert_layer.h"
#include "daily_progress.h"
#include "daily_progress_dialog.h"
#include "daily_progress_label.h"
#include "dictionary_dialog.h"
#include "document.h"
#include "document_cache.h"
#include "document_watcher.h"
#include "format_manager.h"
#include "load_screen.h"
#include "locale_dialog.h"
#include "preferences.h"
#include "preferences_dialog.h"
#include "session.h"
#include "session_manager.h"
#include "smart_quotes.h"
#include "sound.h"
#include "stack.h"
#include "symbols_dialog.h"
#include "theme.h"
#include "theme_manager.h"
#include "timer_display.h"
#include "timer_manager.h"
#include "utils.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileOpenEvent>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSettings>
#include <QSizeGrip>
#include <QStandardPaths>
#include <QTabBar>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

//-----------------------------------------------------------------------------

namespace
{
	QKeySequence keyBinding(const QKeySequence& shortcut, const QKeySequence& back)
	{
		if (!shortcut.isEmpty()) {
			return shortcut;
		} else {
			return back;
		}
	}
}

//-----------------------------------------------------------------------------

Window::Window(const QStringList& command_line_files) :
	m_toolbar(0),
	m_loading(false),
	m_key_sound(0),
	m_enter_key_sound(0),
	m_fullscreen(true),
	m_auto_save(true),
	m_save_positions(true)
{
	setAcceptDrops(true);
	setAttribute(Qt::WA_DeleteOnClose);
	setContextMenuPolicy(Qt::NoContextMenu);
	setCursor(Qt::WaitCursor);

	m_load_screen = new LoadScreen(this);

	// Set up icons
	if (QIcon::themeName().isEmpty()) {
		QIcon::setThemeName("hicolor");
		setIconSize(QSize(22,22));
	}

	// Create actions manager
	new ActionManager(this);

	// Create window contents first so they stack behind documents
#ifndef Q_OS_MAC
	menuBar()->setNativeMenuBar(false);
#else
	menuBar();
#endif
	m_toolbar = new QToolBar(this);
	m_toolbar->setFloatable(false);
	m_toolbar->setMovable(false);
	addToolBar(m_toolbar);
	QWidget* contents = new QWidget(this);
	setCentralWidget(contents);

	// Set up watcher for documents
	m_document_watcher = new DocumentWatcher(this);
	connect(m_document_watcher, SIGNAL(closeDocument(Document*)), this, SLOT(closeDocument(Document*)));
	connect(m_document_watcher, SIGNAL(showDocument(Document*)), this, SLOT(showDocument(Document*)));

	// Set up thread for caching documents
	m_document_cache = new DocumentCache;
	m_document_cache_thread = new QThread(this);
	m_document_cache->moveToThread(m_document_cache_thread);
	m_document_cache_thread->start();

	// Create documents
	m_documents = new Stack(this);
	m_document_cache->setOrdering(m_documents);
	m_sessions = new SessionManager(this);
	m_timers = new TimerManager(m_documents, this);
	connect(m_documents, SIGNAL(footerVisible(bool)), m_timers->display(), SLOT(setVisible(bool)));
	connect(m_documents, SIGNAL(updateFormatActions()), this, SLOT(updateFormatActions()));
	connect(m_documents, SIGNAL(updateFormatAlignmentActions()), this, SLOT(updateFormatAlignmentActions()));
	connect(m_sessions, SIGNAL(themeChanged(Theme)), m_documents, SLOT(themeSelected(Theme)));

	contents->setMouseTracking(true);
	contents->installEventFilter(m_documents);

	// Set up daily progress tracking
	m_daily_progress = new DailyProgress(this);
	m_daily_progress_dialog = new DailyProgressDialog(m_daily_progress, this);
	connect(m_documents, SIGNAL(footerVisible(bool)), m_daily_progress, SLOT(setProgressEnabled(bool)));
	connect(m_daily_progress_dialog, SIGNAL(visibleChanged(bool)), m_daily_progress, SLOT(setProgressEnabled(bool)));

	// Set up menubar and toolbar
	initMenus();

	// Set up cache timer
	m_save_timer = new QTimer(this);
	m_save_timer->setInterval(300000);
	connect(m_save_timer, SIGNAL(timeout()), m_daily_progress, SLOT(save()));

	// Set up details
	m_footer = new QWidget(contents);
	QWidget* details = new QWidget(m_footer);
	m_wordcount_label = new QLabel(tr("Words: %L1").arg(0), details);
	m_page_label = new QLabel(tr("Pages: %L1").arg(0), details);
	m_paragraph_label = new QLabel(tr("Paragraphs: %L1").arg(0), details);
	m_character_label = new QLabel(tr("Characters: %L1 / %L2").arg(0).arg(0), details);
	m_progress_label = new DailyProgressLabel(m_daily_progress, details);
	m_clock_label = new QLabel(details);
	updateClock();

	// Set up clock
	m_clock_timer = new QTimer(this);
	m_clock_timer->setInterval(60000);
	connect(m_clock_timer, SIGNAL(timeout()), this, SLOT(updateClock()));
	connect(m_clock_timer, SIGNAL(timeout()), m_timers, SLOT(saveTimers()));
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
	connect(m_documents, SIGNAL(documentSelected(int)), m_tabs, SLOT(setCurrentIndex(int)));

	QToolButton* tabs_menu = new QToolButton(m_tabs);
	tabs_menu->setArrowType(Qt::UpArrow);
	tabs_menu->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	tabs_menu->setPopupMode(QToolButton::InstantPopup);
	tabs_menu->setMenu(m_documents->menu());
	tabs_menu->setStyleSheet("QToolButton::menu-indicator { image: none; }");
	tabs_menu->setToolTip(tr("List all documents"));

	// Set up tab navigation
	QAction* action = new QAction(tr("Switch to Next Document"), this);
	action->setShortcut(QKeySequence::NextChild);
	connect(action, SIGNAL(triggered()), this, SLOT(nextDocument()));
	addAction(action);
	ActionManager::instance()->addAction("SwitchNextDocument", action);

	action = new QAction(tr("Switch to Previous Document"), this);
	action->setShortcut(QKeySequence::PreviousChild);
	connect(action, SIGNAL(triggered()), this, SLOT(previousDocument()));
	addAction(action);
	ActionManager::instance()->addAction("SwitchPreviousDocument", action);

	action = new QAction(tr("Switch to First Document"), this);
	action->setShortcut(Qt::CTRL + Qt::Key_1);
	connect(action, SIGNAL(triggered()), this, SLOT(firstDocument()));
	addAction(action);
	ActionManager::instance()->addAction("SwitchFirstDocument", action);

	action = new QAction(tr("Switch to Last Document"), this);
	action->setShortcut(Qt::CTRL + Qt::Key_0);
	connect(action, SIGNAL(triggered()), this, SLOT(lastDocument()));
	addAction(action);
	ActionManager::instance()->addAction("SwitchLastDocument", action);

	for (int i = 2; i < 10 ; ++i) {
		action = new QAction(tr("Switch to Document %1").arg(i), this);
		action->setShortcut(Qt::CTRL + Qt::Key_0 + i);
		connect(action, &QAction::triggered, [=] { m_tabs->setCurrentIndex(i - 1); });
		addAction(action);
		ActionManager::instance()->addAction(QString("SwitchDocument%1").arg(i), action);
	}

	// Always bring interface to front
	connect(m_documents, SIGNAL(headerVisible(bool)), menuBar(), SLOT(raise()));
	connect(m_documents, SIGNAL(headerVisible(bool)), m_toolbar, SLOT(raise()));
	connect(m_documents, SIGNAL(footerVisible(bool)), m_footer, SLOT(raise()));

	// Lay out details
	QHBoxLayout* clock_layout = new QHBoxLayout;
	clock_layout->setMargin(0);
	clock_layout->setSpacing(6);
	clock_layout->addWidget(m_timers->display(), 0, Qt::AlignCenter);
	clock_layout->addWidget(m_clock_label);

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
	details_layout->addLayout(clock_layout);

	// Lay out footer
	QGridLayout* footer_layout = new QGridLayout(m_footer);
	footer_layout->setSpacing(0);
	footer_layout->setMargin(0);
	footer_layout->setColumnStretch(0, 1);
	footer_layout->addWidget(details, 0, 0, 1, 3);
	footer_layout->addWidget(m_tabs, 1, 0, 1, 1);
	footer_layout->addWidget(tabs_menu, 1, 1, 1, 1);
	footer_layout->addWidget(new QSizeGrip(this), 1, 2, 1, 1, Qt::AlignBottom);

	// Lay out window
	QVBoxLayout* layout = new QVBoxLayout(contents);
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addStretch();
	layout->addWidget(m_footer);

	// Restore window geometry
	QSettings settings;
	resize(1280, 720);
	restoreGeometry(settings.value("Window/Geometry").toByteArray());
	show();
	m_fullscreen = !settings.value("Window/Fullscreen", true).toBool();
	toggleFullscreen();
	m_actions["Fullscreen"]->setChecked(m_fullscreen);

	// Load settings
	m_load_screen->setText(tr("Loading settings"));
	loadPreferences();

	// Update and load theme
	m_load_screen->setText(tr("Loading themes"));
	Theme::copyBackgrounds();
	{
		// Force a reload of previews
		ThemeManager manager(settings);
	}

	// Update margin
	m_tabs->blockSignals(true);
	m_tabs->addTab(tr("Untitled"));
	updateMargin();
	m_tabs->removeTab(0);
	m_tabs->blockSignals(false);

	// Restore after crash
	QStringList files, datafiles;
	if (!m_document_cache->isWritable()) {
		// Warn user that cache can't be used
		m_documents->alerts()->addAlert(new Alert(Alert::Critical, tr("Emergency cache is not writable."), QStringList(), true));
	} else if (!m_document_cache->isClean()) {
		// Read mapping of cached files
		m_document_cache->parseMapping(files, datafiles);

		// Ask if they want to use cached files
		if (!files.isEmpty()) {
			QStringList filenames;
			int untitled = 1;
			int count = files.count();
			for (int i = 0; i < count; ++i) {
				if (!files.at(i).isEmpty()) {
					filenames.append(QDir::toNativeSeparators(files.at(i)));
				} else {
					filenames.append(tr("(Untitled %1)").arg(untitled));
					untitled++;
				}
			}
			m_load_screen->setText("");
			QMessageBox mbox(window());
			mbox.setWindowTitle(tr("Warning"));
			mbox.setText(tr("FocusWriter was not shut down cleanly."));
			mbox.setInformativeText(tr("Restore from the emergency cache?"));
			mbox.setDetailedText(filenames.join("\n"));
			mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			mbox.setDefaultButton(QMessageBox::Yes);
			mbox.setIcon(QMessageBox::Warning);
			if (mbox.exec() == QMessageBox::No) {
				files.clear();
				datafiles.clear();
			}
		}
	}

	// Open previous documents
	QString session = settings.value("SessionManager/Session").toString();
	if (files.isEmpty() && !command_line_files.isEmpty()) {
		session.clear();
		settings.setValue("Save/Current", settings.value("Save/Current").toStringList() + command_line_files);
	}
	m_sessions->setCurrent(session, files, datafiles);

	// Prevent tabs menu from increasing height
	tabs_menu->setMaximumHeight(m_tabs->sizeHint().height());

	// Bring to front
	activateWindow();
	raise();
	unsetCursor();

	m_save_timer->start();
}

//-----------------------------------------------------------------------------

void Window::addDocuments(const QStringList& files, const QStringList& datafiles, const QStringList& positions, int active, bool show_load)
{
	m_loading = true;

	// Hide interface
	m_documents->setHeaderVisible(false);
	m_documents->setFooterVisible(false);

	// Skip loading files of unsupported formats
	static const QStringList suffixes = QStringList()
		<< "abw" << "awt" << "zabw" << "doc" << "dot" << "docm" << "dotx" << "dotm" << "kwd" << "ott" << "wpd"
		<< "bmp" << "dds" << "gif" << "icns" << "ico" << "jng" << "jp2" << "jpg" << "jpeg" << "jps" << "mng" << "png" << "tga" << "tif" << "tiff" << "xcf"
		<< "aac" << "aif" << "aifc" << "aiff" << "asf" << "au" << "flac" << "mid" << "midi" << "mod" << "mp2" << "mp3" << "m4a" << "ogg" << "s3m" << "snd" << "spx" << "wav" << "wma"
		<< "avi" << "m1v" << "m2ts" << "m4v" << "mkv" << "mov" << "mp4" << "mp4v" << "mpa" << "mpe" << "mpg" << "mpeg" << "mpv2" << "wm" << "wmv";
	QList<int> skip;
	for (int i = 0; i < files.count(); ++i) {
		if (suffixes.contains(QFileInfo(files.at(i)).suffix().toLower())) {
			skip += i;
		}
	}
	if (!skip.isEmpty()) {
		QStringList skipped;
		for (int i : skip) {
			skipped += QDir::toNativeSeparators(files.at(i));
		}
		m_documents->alerts()->addAlert(new Alert(Alert::Warning, tr("Some files were unsupported and could not be opened."), skipped, true));
	}

	// Show load screen if switching sessions or opening more than one file
	show_load = show_load || ((files.count() > 1) && (files.count() > skip.count()));
	if (show_load) {
		m_load_screen->setText("");
		setCursor(Qt::WaitCursor);
	}

	// Remember current file and if it is untitled and unmodified
	int untitled_index = -1;
	int current_index = -1;
	if (m_documents->count()) {
		current_index = m_documents->currentIndex();
		Document* document = m_documents->currentDocument();
		if (document->untitledIndex() && !document->isModified()) {
			untitled_index = m_documents->currentIndex();
		}
	}

	// Read files
	QStringList missing;
	QStringList errors;
	QStringList readonly;
	int open_files = m_documents->count();
	for (int i = 0; i < files.count(); ++i) {
		// Skip file known to be unsupported
		if (!skip.isEmpty() && (skip.first() == i)) {
			skip.removeFirst();
			continue;
		// Attempt to load file or datafile
		} else if (!addDocument(files.at(i), datafiles.at(i), positions.value(i, "-1").toInt())) {
			// Track if unable to open file
			missing.append(QDir::toNativeSeparators(files.at(i)));
		} else if (!files.at(i).isEmpty() && (m_documents->currentDocument()->untitledIndex() > 0)) {
			// Track if unable to read file
			int index = m_documents->currentIndex();
			errors.append(QDir::toNativeSeparators(files.at(i)));
			closeDocument(index, true);
		} else if (m_documents->currentDocument()->isReadOnly() && (m_documents->count() > open_files)) {
			// Track if file is read-only and not already open
			readonly.append(QDir::toNativeSeparators(files.at(i)));
		}
		open_files = m_documents->count();
	}

	// Make sure that there is always at least one document
	if (m_documents->count() == 0) {
		newDocument();
	}

	// Switch to tab of active session file or of current file
	if (untitled_index == -1) {
		if (active != -1) {
			m_tabs->setCurrentIndex(active);
		} else if (m_documents->currentIndex() == current_index) {
			m_tabs->setCurrentIndex(m_tabs->count() - 1);
		}
	// Replace current tab if it is untitled and unmodified
	} else if (files.count() > (missing.count() + errors.count())) {
		m_tabs->setCurrentIndex(untitled_index);
		closeDocument();
	}

	// Inform user about unopened and read-only files
	if (!missing.isEmpty()) {
		m_documents->alerts()->addAlert(new Alert(Alert::Warning, tr("Some files could not be opened."), missing, true));
	}
	if (!readonly.isEmpty()) {
		m_documents->alerts()->addAlert(new Alert(Alert::Information, tr("Some files were opened Read-Only."), readonly, true));
	}

	// Hide load screen
	if (m_load_screen->isVisible()) {
		m_documents->waitForThemeBackground();
		m_load_screen->finish();
		unsetCursor();
	}

	// Open any files queued during load
	if (!m_queued_documents.isEmpty()) {
		QStringList queued = m_queued_documents;
		m_queued_documents.clear();
		addDocuments(queued, queued);
	}
	m_loading = false;
}

//-----------------------------------------------------------------------------

void Window::addDocuments(QDropEvent* event)
{
	if (event->mimeData()->hasUrls()) {
		QStringList files;
		for (const QUrl& url : event->mimeData()->urls()) {
			files.append(url.toLocalFile());
		}
		queueDocuments(files);
		event->acceptProposedAction();
	}
}

//-----------------------------------------------------------------------------

bool Window::closeDocuments(QSettings* session)
{
	// Save files
	if (!saveDocuments(session)) {
		return false;
	}

	// Close files
	int count = m_documents->count();
	for (int i = 0; i < count; ++i) {
		closeDocument(0, true);
	}

	return true;
}

//-----------------------------------------------------------------------------

bool Window::saveDocuments(QSettings* session)
{
	if (m_documents->count() == 0) {
		return true;
	}

	// Save files
	int active = m_tabs->currentIndex();
	QStringList files;
	QStringList positions;
	for (int i = 0; i < m_documents->count(); ++i) {
		m_tabs->setCurrentIndex(i);
		if (!saveDocument(i)) {
			m_tabs->setCurrentIndex(active);
			return false;
		}

		Document* document = m_documents->document(i);
		QString filename = document->filename();
		if (!filename.isEmpty()) {
			files.append(filename);
			positions.append(QString::number(document->text()->textCursor().position()));
		}
	}

	// Store current files
	session->setValue("Save/Current", files);
	session->setValue("Save/Positions", positions);
	session->setValue("Save/Active", active);

	return true;
}

//-----------------------------------------------------------------------------

void Window::addDocuments(const QString& documents)
{
	QStringList files = documents.split(QLatin1String("\n"), QString::SkipEmptyParts);
	if (!files.isEmpty()) {
		queueDocuments(files);
	}
}

//-----------------------------------------------------------------------------

void Window::changeEvent(QEvent* event)
{
	if (isActiveWindow()) {
		m_document_watcher->processUpdates();
	}
	QMainWindow::changeEvent(event);
}

//-----------------------------------------------------------------------------

void Window::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	}
}

//-----------------------------------------------------------------------------

void Window::dropEvent(QDropEvent* event)
{
	addDocuments(event);
}

//-----------------------------------------------------------------------------

bool Window::event(QEvent* event)
{
	if (event->type() == QEvent::WindowBlocked) {
		hideInterface();
	}
	return QMainWindow::event(event);
}

//-----------------------------------------------------------------------------

void Window::closeEvent(QCloseEvent* event)
{
	// Confirm discarding any unsaved changes
	if (!m_timers->cancelEditing() || !m_sessions->saveCurrent()) {
		event->ignore();
		return;
	}

	// Close documents but keep them cached
	int count = m_documents->count();
	for (int i = 0; i < count; ++i) {
		m_documents->removeDocument(0);
	}

	// Save window settings
	QSettings().setValue("Window/FocusedText", m_focus_actions->checkedAction()->data().toInt());
	if (!m_fullscreen) {
		QSettings().setValue("Window/Geometry", saveGeometry());
	}

	// Stop cache thread while window is visible
	setCursor(Qt::WaitCursor);
	m_document_cache_thread->quit();
	m_document_cache_thread->wait();
	delete m_document_cache;
	m_document_cache = 0;

	QMainWindow::closeEvent(event);
}

//-----------------------------------------------------------------------------

void Window::leaveEvent(QEvent* event)
{
	if ((qApp->activePopupWidget() == 0) && !m_fullscreen) {
		hideInterface();
	}
	QMainWindow::leaveEvent(event);
}

//-----------------------------------------------------------------------------

void Window::resizeEvent(QResizeEvent* event)
{
	if (!m_fullscreen) {
		QSettings().setValue("Window/Geometry", saveGeometry());
	}
	m_load_screen->resize(size());
	m_documents->resize(size());
	QMainWindow::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void Window::newDocument()
{
	addDocument();
	m_actions["Rename"]->setEnabled(false);
	if (m_documents->currentDocument()->isRichText()) {
		if (QApplication::isLeftToRight()) {
			m_actions["FormatDirectionLTR"]->setChecked(true);
		} else {
			m_actions["FormatDirectionRTL"]->setChecked(true);
		}
	}
}

//-----------------------------------------------------------------------------

void Window::openDocument()
{
	QSettings settings;
	QString default_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	QString path = settings.value("Save/Location", default_path).toString();

	QStringList filenames = QFileDialog::getOpenFileNames(window(), tr("Open File"), path, FormatManager::filters().join(";;"), 0, QFileDialog::DontResolveSymlinks);
	if (!filenames.isEmpty()) {
		addDocuments(filenames, filenames);
		settings.setValue("Save/Location", QFileInfo(filenames.last()).absolutePath());
	}
}

//-----------------------------------------------------------------------------

void Window::renameDocument()
{
	if (m_documents->currentDocument()->rename()) {
		updateTab(m_documents->currentIndex());
	}
}

//-----------------------------------------------------------------------------

void Window::saveAllDocuments()
{
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

//-----------------------------------------------------------------------------

void Window::closeDocument()
{
	int index = m_documents->currentIndex();
	if (!saveDocument(index)) {
		return;
	}
	closeDocument(index);
}

//-----------------------------------------------------------------------------

void Window::closeDocument(Document* document)
{
	for (int i = 0; i < m_documents->count(); ++i) {
		if (m_documents->document(i) == document) {
			return closeDocument(i);
		}
	}
}

//-----------------------------------------------------------------------------

void Window::showDocument(Document* document)
{
	for (int i = 0; i < m_documents->count(); ++i) {
		if (m_documents->document(i) == document) {
			m_tabs->setCurrentIndex(i);
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void Window::nextDocument()
{
	int index = m_tabs->currentIndex() + 1;
	if (index >= m_tabs->count()) {
		index = 0;
	}
	m_tabs->setCurrentIndex(index);
}

//-----------------------------------------------------------------------------

void Window::previousDocument()
{
	int index = m_tabs->currentIndex() - 1;
	if (index < 0) {
		index = m_tabs->count() - 1;
	}
	m_tabs->setCurrentIndex(index);
}

//-----------------------------------------------------------------------------

void Window::firstDocument()
{
	m_tabs->setCurrentIndex(0);
}

//-----------------------------------------------------------------------------

void Window::lastDocument()
{
	m_tabs->setCurrentIndex(m_tabs->count() - 1);
}

//-----------------------------------------------------------------------------

void Window::setLanguageClicked()
{
	DictionaryDialog dialog(this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::minimize()
{
#ifdef Q_OS_MAC
	if (m_fullscreen) {
		toggleFullscreen();
	}
#endif
	showMinimized();
}

//-----------------------------------------------------------------------------

void Window::toggleFullscreen()
{
	m_fullscreen = !m_fullscreen;
	QSettings().setValue("Window/Fullscreen", m_fullscreen);

	if (m_fullscreen) {
		setWindowState(windowState() | Qt::WindowFullScreen);
	} else {
		setWindowState(windowState() & ~Qt::WindowFullScreen);
	}
	show();
	QApplication::processEvents();
	activateWindow();
	raise();
	QApplication::processEvents();
}

//-----------------------------------------------------------------------------

void Window::toggleToolbar(bool visible)
{
	m_toolbar->setVisible(visible);
	QSettings().setValue("Toolbar/Shown", visible);
	updateMargin();
}

//-----------------------------------------------------------------------------

void Window::toggleMenuIcons(bool visible)
{
	QApplication::setAttribute(Qt::AA_DontShowIconsInMenus, !visible);
	QSettings().setValue("Window/MenuIcons", visible);
}

//-----------------------------------------------------------------------------

void Window::themeClicked()
{
	ThemeManager manager(*m_sessions->current()->data(), this);
	connect(&manager, SIGNAL(themeSelected(Theme)), m_documents, SLOT(themeSelected(Theme)));
	manager.exec();
}

//-----------------------------------------------------------------------------

void Window::preferencesClicked()
{
	PreferencesDialog dialog(m_daily_progress, this);
	if (dialog.exec() == QDialog::Accepted) {
		loadPreferences();
	}
}

//-----------------------------------------------------------------------------

void Window::aboutClicked()
{
	QMessageBox::about(this, tr("About FocusWriter"), QString(
		"<p align='center'><big><b>%1 %2</b></big><br/>%3<br/><small>%4<br/>%5</small></p>"
		"<p align='center'>%6<br/><small>%7</small></p>")
		.arg(tr("FocusWriter"), QApplication::applicationVersion(),
			tr("A simple fullscreen word processor"),
			tr("Copyright &copy; 2008-%1 Graeme Gott").arg("2018"),
			tr("Released under the <a href=%1>GPL 3</a> license").arg("\"http://www.gnu.org/licenses/gpl.html\""),
			tr("Uses icons from the <a href=%1>Oxygen</a> icon theme").arg("\"http://www.oxygen-icons.org/\""),
			tr("Used under the <a href=%1>LGPL 3</a> license").arg("\"http://www.gnu.org/licenses/lgpl.html\""))
	);
}

//-----------------------------------------------------------------------------

void Window::setLocaleClicked()
{
	LocaleDialog dialog(this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::tabClicked(int index)
{
	if (m_documents->count() == 0) {
		return;
	}
	updateWriteState(index);
	m_documents->setCurrentDocument(index);
	updateDetails();
	updateSave();
	updateFormatAlignmentActions();
	m_documents->currentDocument()->text()->setFocus();
}

//-----------------------------------------------------------------------------

void Window::tabClosed(int index)
{
	m_tabs->setCurrentIndex(index);
	closeDocument();
}

//-----------------------------------------------------------------------------

void Window::tabMoved(int from, int to)
{
	m_documents->moveDocument(from, to);
	m_documents->setCurrentDocument(m_tabs->currentIndex());
	m_document_cache->updateMapping();
}

//-----------------------------------------------------------------------------

void Window::updateClock()
{
	m_clock_label->setText(QTime::currentTime().toString(Qt::DefaultLocaleShortDate));
}

//-----------------------------------------------------------------------------

void Window::updateDetails()
{
	Document* document = m_documents->currentDocument();
	m_character_label->setText(tr("Characters: %L1 / %L2").arg(document->characterCount()).arg(document->characterAndSpaceCount()));
	m_page_label->setText(tr("Pages: %L1").arg(document->pageCount()));
	m_paragraph_label->setText(tr("Paragraphs: %L1").arg(document->paragraphCount()));
	m_wordcount_label->setText(tr("Words: %L1").arg(document->wordCount()));
}

//-----------------------------------------------------------------------------

void Window::updateFormatActions()
{
	Document* document = m_documents->currentDocument();
	if (!document) {
		return;
	}

	m_actions["FormatIndentDecrease"]->setEnabled(!document->isReadOnly() && document->text()->textCursor().blockFormat().indent() > 0);

	QTextCharFormat format = document->text()->currentCharFormat();
	m_actions["FormatBold"]->setChecked(format.fontWeight() == QFont::Bold);
	m_actions["FormatItalic"]->setChecked(format.fontItalic());
	m_actions["FormatStrikeOut"]->setChecked(format.fontStrikeOut());
	m_actions["FormatUnderline"]->setChecked(format.fontUnderline());
	m_actions["FormatSuperScript"]->setChecked(format.verticalAlignment() == QTextCharFormat::AlignSuperScript);
	m_actions["FormatSubScript"]->setChecked(format.verticalAlignment() == QTextCharFormat::AlignSubScript);
}

//-----------------------------------------------------------------------------

void Window::updateFormatAlignmentActions()
{
	Document* document = m_documents->currentDocument();
	if (!document) {
		return;
	}

	QTextBlockFormat format = document->text()->textCursor().blockFormat();

	if (format.layoutDirection() == Qt::LeftToRight) {
		m_actions["FormatDirectionLTR"]->setChecked(true);
	} else if (document->text()->textCursor().blockFormat().layoutDirection() == Qt::RightToLeft) {
		m_actions["FormatDirectionRTL"]->setChecked(true);
	} else {
		m_actions[QApplication::isLeftToRight() ? "FormatDirectionLTR" : "FormatDirectionRTL"]->setChecked(true);
	}

	Qt::Alignment alignment = document->text()->alignment();
	if (alignment & Qt::AlignLeft) {
		m_actions["FormatAlignLeft"]->setChecked(true);
	} else if (alignment & Qt::AlignRight) {
		m_actions["FormatAlignRight"]->setChecked(true);
	} else if (alignment & Qt::AlignCenter) {
		m_actions["FormatAlignCenter"]->setChecked(true);
	} else if (alignment & Qt::AlignJustify) {
		m_actions["FormatAlignJustify"]->setChecked(true);
	}

	int heading = format.property(QTextFormat::UserProperty).toInt();
	m_headings_actions->actions().at(heading)->setChecked(true);
}

//-----------------------------------------------------------------------------

void Window::updateSave()
{
	m_actions["Save"]->setEnabled(m_documents->currentDocument()->isModified());
	m_actions["Rename"]->setDisabled(m_documents->currentDocument()->isReadOnly() || m_documents->currentDocument()->filename().isEmpty());
	for (int i = 0; i < m_documents->count(); ++i) {
		updateTab(i);
	}
}

//-----------------------------------------------------------------------------

bool Window::addDocument(const QString& file, const QString& datafile, int position)
{
	QFileInfo info(file);
	if (!file.isEmpty()) {
		// Check if already open
		QString canonical_filename = info.canonicalFilePath();
		for (int i = 0; i < m_documents->count(); ++i) {
			if (QFileInfo(m_documents->document(i)->filename()).canonicalFilePath() == canonical_filename) {
				m_tabs->setCurrentIndex(i);
				return true;
			}
		}

		// Check if unreadable
		if (!info.exists() || !info.isReadable()) {
			return false;
		}
	}

	// Show filename in load screen
	bool show_load = false;
	show_load = !file.isEmpty() && !m_load_screen->isVisible() && (info.size() > 100000);
	if (m_load_screen->isVisible() || show_load) {
		if (!file.isEmpty()) {
			m_load_screen->setText(tr("Opening %1").arg(QDir::toNativeSeparators(file)));
		} else {
			m_load_screen->setText("");
		}
	}

	// Create document
	QString path = file;
	if (!file.isEmpty() && (datafile != file)) {
		if (QFileInfo(datafile).lastModified() > QFileInfo(file).lastModified()) {
			path = datafile;
			position = -1;
		} else {
			QMessageBox mbox(window());
			mbox.setWindowTitle(tr("Warning"));
			mbox.setText(tr("'%1' is newer than the cached copy.").arg(file));
			mbox.setInformativeText(tr("Overwrite newer file?"));
			mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			mbox.setDefaultButton(QMessageBox::No);
			mbox.setIcon(QMessageBox::Warning);
			if (mbox.exec() == QMessageBox::Yes) {
				path = datafile;
				position = -1;
			}
		}
	} else {
		path = datafile;
	}
	Document* document = new Document(file, m_daily_progress, this);
	m_documents->addDocument(document);
	m_document_cache->add(document);
	document->setFocusMode(m_focus_actions->checkedAction()->data().toInt());
	if (document->loadFile(path, m_save_positions ? position : -1)) {
		if (datafile != file) {
			document->setModified(!compareFiles(file, datafile));
		}
	} else if (path != file) {
		document->loadFile(file, m_save_positions ? position : -1);
	}
	connect(document, SIGNAL(changed()), this, SLOT(updateDetails()));
	connect(document, SIGNAL(changedName()), this, SLOT(updateSave()));
	connect(document, SIGNAL(indentChanged(bool)), m_actions["FormatIndentDecrease"], SLOT(setEnabled(bool)));
	connect(document, SIGNAL(modificationChanged(bool)), this, SLOT(updateSave()));

	// Add tab for document
	int index = m_tabs->addTab(tr("Untitled"));
	updateTab(index);
	m_tabs->setCurrentIndex(index);

	if (show_load) {
		m_load_screen->finish();
	}

	// Allow documents to show load screen on reload
	connect(document, SIGNAL(loadStarted(QString)), m_load_screen, SLOT(setText(QString)));
	connect(document, SIGNAL(loadFinished()), m_load_screen, SLOT(finish()));
	connect(document, SIGNAL(loadFinished()), this, SLOT(updateSave()));

	return true;
}

//-----------------------------------------------------------------------------

void Window::closeDocument(int index, bool allow_empty)
{
	m_document_cache->remove(m_documents->document(index));
	m_documents->removeDocument(index);
	m_tabs->removeTab(index);

	if (!allow_empty && !m_documents->count()) {
		newDocument();
	}
}

//-----------------------------------------------------------------------------

void Window::queueDocuments(const QStringList& files)
{
	if (m_loading) {
		m_queued_documents += files;
	} else {
		addDocuments(files, files);
	}
}

//-----------------------------------------------------------------------------

bool Window::saveDocument(int index)
{
	Document* document = m_documents->document(index);
	if (!document->isModified()) {
		return true;
	}

	// Auto-save document
	if (m_auto_save && document->isModified() && !document->filename().isEmpty()) {
		return document->save();
	}

	// Prompt about saving changes
	QMessageBox mbox(window());
	mbox.setWindowTitle(tr("Save Changes?"));
	mbox.setText(tr("Save changes to the file '%1' before closing?").arg(document->title()));
	mbox.setInformativeText(tr("Your changes will be lost if you don't save them."));
	mbox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	mbox.setDefaultButton(QMessageBox::Save);
	mbox.setIcon(QMessageBox::Warning);
	switch (mbox.exec()) {
	case QMessageBox::Save:
		return document->save();
	case QMessageBox::Discard:
		document->setModified(false);
		m_daily_progress->increaseWordCount(-document->wordCountDelta());
		return true;
	case QMessageBox::Cancel:
	default:
		return false;
	}
}

//-----------------------------------------------------------------------------

void Window::loadPreferences()
{
	if (Preferences::instance().typewriterSounds() && (!m_key_sound || !m_enter_key_sound)) {
		if (m_load_screen->isVisible()) {
			m_load_screen->setText(tr("Loading sounds"));
		}
		m_key_sound = new Sound(Qt::Key_Any, "keyany.wav", this);
		m_enter_key_sound = new Sound(Qt::Key_Enter, "keyenter.wav", this);

		if (!m_key_sound->isValid() || !m_enter_key_sound->isValid()) {
			m_documents->alerts()->addAlert(new Alert(Alert::Warning,
				tr("Unable to load typewriter sounds."),
				QStringList(),
				false));
			delete m_key_sound;
			delete m_enter_key_sound;
			m_key_sound = m_enter_key_sound = 0;
			Preferences::instance().setTypewriterSounds(false);
		}
	}
	Sound::setEnabled(Preferences::instance().typewriterSounds());

	m_auto_save = Preferences::instance().autoSave();
	if (m_auto_save) {
		disconnect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoCache()));
		connect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoSave()));
	} else {
		disconnect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoSave()));
		connect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoCache()));
	}
	m_save_positions = Preferences::instance().savePositions();

	SmartQuotes::loadPreferences();

	m_character_label->setVisible(Preferences::instance().showCharacters());
	m_page_label->setVisible(Preferences::instance().showPages());
	m_paragraph_label->setVisible(Preferences::instance().showParagraphs());
	m_wordcount_label->setVisible(Preferences::instance().showWords());
	m_progress_label->setVisible(Preferences::instance().goalType() != 0);

	m_daily_progress->loadPreferences();
	m_daily_progress_dialog->loadPreferences();
	m_actions["DailyProgress"]->setEnabled(Preferences::instance().goalHistory());
	if (Preferences::instance().goalHistory()) {
		connect(m_progress_label, SIGNAL(clicked()), m_actions["DailyProgress"], SLOT(trigger()));
	} else {
		disconnect(m_progress_label, SIGNAL(clicked()), m_actions["DailyProgress"], SLOT(trigger()));
		m_daily_progress_dialog->hide();
	}

	m_toolbar->clear();
	m_toolbar->hide();
	m_toolbar->setToolButtonStyle(Qt::ToolButtonStyle(Preferences::instance().toolbarStyle()));
	QStringList actions = Preferences::instance().toolbarActions();
	for (const QString action : actions) {
		if (action == "|") {
			m_toolbar->addSeparator();
		} else if (!action.startsWith("^")) {
			m_toolbar->addAction(m_actions.value(action));
		}
	}
	m_toolbar->setVisible(QSettings().value("Toolbar/Shown", true).toBool());
	updateMargin();

	m_documents->setHeaderVisible(Preferences::instance().alwaysShowHeader());
	m_documents->setFooterVisible(Preferences::instance().alwaysShowFooter());
	for (int i = 0; i < m_documents->count(); ++i) {
		m_documents->document(i)->loadPreferences();
	}
	if (m_documents->count() > 0) {
		updateDetails();
	}

	m_replace_document_quotes->setEnabled(Preferences::instance().smartQuotes());
	m_replace_selection_quotes->setEnabled(Preferences::instance().smartQuotes());
}

//-----------------------------------------------------------------------------

void Window::hideInterface()
{
	m_documents->setFooterVisible(false);
	m_documents->setHeaderVisible(false);
	m_documents->setScenesVisible(false);
	for (int i = 0; i < m_documents->count(); ++i) {
		m_documents->document(i)->setScrollBarVisible(false);
	}
}

//-----------------------------------------------------------------------------

void Window::updateMargin()
{
	QApplication::processEvents();
	int header = centralWidget()->mapToParent(QPoint(0,0)).y();
	int footer = m_footer->sizeHint().height();
	m_documents->setMargins(footer, header);
}

//-----------------------------------------------------------------------------

void Window::updateTab(int index)
{
	Document* document = m_documents->document(index);
	QString name = document->title();
	m_tabs->setTabText(index, name + (document->isModified() ? "*" : ""));
	m_tabs->setTabToolTip(index, QDir::toNativeSeparators(document->filename()));
	m_documents->updateDocument(index);
	if (document == m_documents->currentDocument()) {
		setWindowFilePath(name);
		setWindowModified(document->isModified());
		updateWriteState(index);
	}
}

//-----------------------------------------------------------------------------

void Window::updateWriteState(int index)
{
	Document* document = m_documents->document(index);

	bool writable = !document->isReadOnly();
	m_actions["Paste"]->setEnabled(writable);
	m_actions["PasteUnformatted"]->setEnabled(writable);
	m_actions["Replace"]->setEnabled(writable);
	m_actions["CheckSpelling"]->setEnabled(writable);
	if (writable) {
		connect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Cut"], SLOT(setEnabled(bool)));
	} else {
		disconnect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Cut"], SLOT(setEnabled(bool)));
		m_actions["Cut"]->setEnabled(false);
	}
	m_replace_document_quotes->setEnabled(writable);
	m_replace_selection_quotes->setEnabled(writable);

	if (m_documents->symbols()) {
		m_documents->symbols()->setInsertEnabled(writable);
	}

	for (QAction* action : m_format_actions) {
		action->setEnabled(writable);
	}
	m_actions["FormatIndentDecrease"]->setEnabled(writable && document->text()->textCursor().blockFormat().indent() > 0);
}

//-----------------------------------------------------------------------------

void Window::initMenus()
{
	// Create file menu
	QMenu* file_menu = menuBar()->addMenu(tr("&File"));
	m_actions["New"] = file_menu->addAction(QIcon::fromTheme("document-new"), tr("&New"), this, SLOT(newDocument()), QKeySequence::New);
	m_actions["Open"] = file_menu->addAction(QIcon::fromTheme("document-open"), tr("&Open..."), this, SLOT(openDocument()), QKeySequence::Open);
	m_actions["Reload"] = file_menu->addAction(QIcon::fromTheme("view-refresh"), tr("Reloa&d"), m_documents, SLOT(reload()), QKeySequence::Refresh);
	file_menu->addSeparator();
	m_actions["Save"] = file_menu->addAction(QIcon::fromTheme("document-save"), tr("&Save"), m_documents, SLOT(save()), QKeySequence::Save);
	m_actions["Save"]->setEnabled(false);
	m_actions["SaveAs"] = file_menu->addAction(QIcon::fromTheme("document-save-as"), tr("Save &As..."), m_documents, SLOT(saveAs()), QKeySequence::SaveAs);
	m_actions["Rename"] = file_menu->addAction(QIcon::fromTheme("edit-rename"), tr("&Rename..."), this, SLOT(renameDocument()));
	m_actions["Rename"]->setEnabled(false);
	m_actions["SaveAll"] = file_menu->addAction(QIcon::fromTheme("document-save-all"), tr("Save A&ll"), this, SLOT(saveAllDocuments()));
	file_menu->addSeparator();
	file_menu->addMenu(m_sessions->menu());
	m_actions["ManageSessions"] = new QAction(QIcon::fromTheme("view-choose"), tr("Manage Sessions"), this);
	connect(m_actions["ManageSessions"], SIGNAL(triggered()), m_sessions, SLOT(exec()));
	m_actions["NewSession"] = new QAction(QIcon::fromTheme("window-new"), tr("New Session"), this);
	connect(m_actions["NewSession"], SIGNAL(triggered()), m_sessions, SLOT(newSession()));
	file_menu->addSeparator();
	m_actions["Print"] = file_menu->addAction(QIcon::fromTheme("document-print"), tr("&Print..."), m_documents, SLOT(print()), QKeySequence::Print);
	m_actions["PageSetup"] = file_menu->addAction(QIcon::fromTheme("preferences-desktop-printer"), tr("Pa&ge Setup..."), m_documents, SLOT(pageSetup()));
	file_menu->addSeparator();
	m_actions["Close"] = file_menu->addAction(QIcon::fromTheme("window-close"), tr("&Close"), this, SLOT(closeDocument()), QKeySequence::Close);
	m_actions["Quit"] = file_menu->addAction(QIcon::fromTheme("application-exit"), tr("&Quit"), this, SLOT(close()), keyBinding(QKeySequence::Quit, tr("Ctrl+Q")));
	m_actions["Quit"]->setMenuRole(QAction::QuitRole);

	// Create edit menu
	QMenu* edit_menu = menuBar()->addMenu(tr("&Edit"));
	m_actions["Undo"] = edit_menu->addAction(QIcon::fromTheme("edit-undo"), tr("&Undo"), m_documents, SLOT(undo()), QKeySequence::Undo);
	m_actions["Undo"]->setEnabled(false);
	connect(m_documents, SIGNAL(undoAvailable(bool)), m_actions["Undo"], SLOT(setEnabled(bool)));
	m_actions["Redo"] = edit_menu->addAction(QIcon::fromTheme("edit-redo"), tr("&Redo"), m_documents, SLOT(redo()), QKeySequence::Redo);
	m_actions["Redo"]->setEnabled(false);
	connect(m_documents, SIGNAL(redoAvailable(bool)), m_actions["Redo"], SLOT(setEnabled(bool)));
	edit_menu->addSeparator();
	m_actions["Cut"] = edit_menu->addAction(QIcon::fromTheme("edit-cut"), tr("Cu&t"), m_documents, SLOT(cut()), QKeySequence::Cut);
	m_actions["Cut"]->setEnabled(false);
	connect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Cut"], SLOT(setEnabled(bool)));
	m_actions["Copy"] = edit_menu->addAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), m_documents, SLOT(copy()), QKeySequence::Copy);
	m_actions["Copy"]->setEnabled(false);
	connect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Copy"], SLOT(setEnabled(bool)));
	m_actions["Paste"] = edit_menu->addAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), m_documents, SLOT(paste()), QKeySequence::Paste);
	m_actions["PasteUnformatted"] = edit_menu->addAction(QIcon::fromTheme("edit-paste"), tr("Paste &Unformatted"), m_documents, SLOT(pasteUnformatted()), tr("Ctrl+Shift+V"));
	edit_menu->addSeparator();
	m_actions["SelectAll"] = edit_menu->addAction(QIcon::fromTheme("edit-select-all"), tr("Select &All"), m_documents, SLOT(selectAll()), QKeySequence::SelectAll);
	m_actions["SelectScene"] = edit_menu->addAction(QIcon::fromTheme("edit-select-all"), tr("Select &Scene"), m_documents, SLOT(selectScene()), tr("Ctrl+Shift+A"));

	// Create format menu
	QMenu* format_menu = menuBar()->addMenu(tr("Fo&rmat"));

	QMenu* headings_menu = format_menu->addMenu(tr("&Heading"));
	QAction* headings[7];
	headings[1] = headings_menu->addAction(tr("Heading &1"));
	headings[2] = headings_menu->addAction(tr("Heading &2"));
	headings[3] = headings_menu->addAction(tr("Heading &3"));
	headings[4] = headings_menu->addAction(tr("Heading &4"));
	headings[5] = headings_menu->addAction(tr("Heading &5"));
	headings[6] = headings_menu->addAction(tr("Heading &6"));
	headings[0] = headings_menu->addAction(tr("&Normal"));
	m_headings_actions = new QActionGroup(this);
	for (int i = 0; i < 7; ++i) {
		headings[i]->setCheckable(true);
		headings[i]->setData(i);
		m_headings_actions->addAction(headings[i]);
		connect(headings[i], &QAction::triggered, [=] { m_documents->setBlockHeading(i); });
	}
	for (int i = 1; i < 7; ++i) {
		ActionManager::instance()->addAction(QString("FormatBlockHeading%1").arg(i), headings[i]);
	}
	ActionManager::instance()->addAction(QString("FormatBlockNormal"), headings[0]);
	headings[0]->setChecked(true);

	format_menu->addSeparator();
	m_actions["FormatBold"] = format_menu->addAction(QIcon::fromTheme("format-text-bold"), tr("&Bold"), m_documents, SLOT(setFontBold(bool)), QKeySequence::Bold);
	m_actions["FormatBold"]->setCheckable(true);
	m_actions["FormatItalic"] = format_menu->addAction(QIcon::fromTheme("format-text-italic"), tr("&Italic"), m_documents, SLOT(setFontItalic(bool)), QKeySequence::Italic);
	m_actions["FormatItalic"]->setCheckable(true);
	m_actions["FormatUnderline"] = format_menu->addAction(QIcon::fromTheme("format-text-underline"), tr("&Underline"), m_documents, SLOT(setFontUnderline(bool)), QKeySequence::Underline);
	m_actions["FormatUnderline"]->setCheckable(true);
	m_actions["FormatStrikeOut"] = format_menu->addAction(QIcon::fromTheme("format-text-strikethrough"), tr("Stri&kethrough"), m_documents, SLOT(setFontStrikeOut(bool)), tr("Ctrl+K"));
	m_actions["FormatStrikeOut"]->setCheckable(true);
	m_actions["FormatSuperScript"] = format_menu->addAction(QIcon::fromTheme("format-text-superscript"), tr("Sup&erscript"), m_documents, SLOT(setFontSuperScript(bool)), tr("Ctrl+^"));
	m_actions["FormatSuperScript"]->setCheckable(true);
	m_actions["FormatSubScript"] = format_menu->addAction(QIcon::fromTheme("format-text-subscript"), tr("&Subscript"), m_documents, SLOT(setFontSubScript(bool)), tr("Ctrl+_"));
	m_actions["FormatSubScript"]->setCheckable(true);

	format_menu->addSeparator();
	m_actions["FormatAlignLeft"] = format_menu->addAction(QIcon::fromTheme("format-justify-left"), tr("Align &Left"), m_documents, SLOT(alignLeft()), tr("Ctrl+{"));
	m_actions["FormatAlignLeft"]->setCheckable(true);
	m_actions["FormatAlignCenter"] = format_menu->addAction(QIcon::fromTheme("format-justify-center"), tr("Align &Center"), m_documents, SLOT(alignCenter()), tr("Ctrl+|"));
	m_actions["FormatAlignCenter"]->setCheckable(true);
	m_actions["FormatAlignRight"] = format_menu->addAction(QIcon::fromTheme("format-justify-right"), tr("Align &Right"), m_documents, SLOT(alignRight()), tr("Ctrl+}"));
	m_actions["FormatAlignRight"]->setCheckable(true);
	m_actions["FormatAlignJustify"] = format_menu->addAction(QIcon::fromTheme("format-justify-fill"), tr("Align &Justify"), m_documents, SLOT(alignJustify()), tr("Ctrl+J"));
	m_actions["FormatAlignJustify"]->setCheckable(true);
	QActionGroup* alignment = new QActionGroup(this);
	alignment->addAction(m_actions["FormatAlignLeft"]);
	alignment->addAction(m_actions["FormatAlignCenter"]);
	alignment->addAction(m_actions["FormatAlignRight"]);
	alignment->addAction(m_actions["FormatAlignJustify"]);
	m_actions["FormatAlignLeft"]->setChecked(true);

	format_menu->addSeparator();
	m_actions["FormatIndentDecrease"] = format_menu->addAction(QIcon::fromTheme("format-indent-less"), tr("&Decrease Indent"), m_documents, SLOT(decreaseIndent()), tr("Ctrl+<"));
	m_actions["FormatIndentIncrease"] = format_menu->addAction(QIcon::fromTheme("format-indent-more"), tr("I&ncrease Indent"), m_documents, SLOT(increaseIndent()), tr("Ctrl+>"));

	format_menu->addSeparator();
	m_actions["FormatDirectionLTR"] = format_menu->addAction(QIcon::fromTheme("format-text-direction-ltr"), tr("Le&ft to Right Block"), m_documents, SLOT(setTextDirectionLTR()));
	m_actions["FormatDirectionLTR"]->setCheckable(true);
	m_actions["FormatDirectionRTL"] = format_menu->addAction(QIcon::fromTheme("format-text-direction-rtl"), tr("Ri&ght to Left Block"), m_documents, SLOT(setTextDirectionRTL()));
	m_actions["FormatDirectionRTL"]->setCheckable(true);
	QActionGroup* direction = new QActionGroup(this);
	direction->addAction(m_actions["FormatDirectionLTR"]);
	direction->addAction(m_actions["FormatDirectionRTL"]);
	m_actions["FormatDirectionLTR"]->setChecked(true);

	// Create tools menu
	QMenu* tools_menu = menuBar()->addMenu(tr("&Tools"));
	m_actions["Find"] = tools_menu->addAction(QIcon::fromTheme("edit-find"), tr("&Find..."), m_documents, SLOT(find()), QKeySequence::Find);
	m_actions["FindNext"] = tools_menu->addAction(QIcon::fromTheme("go-down"), tr("Find &Next"), m_documents, SLOT(findNext()), QKeySequence::FindNext);
	m_actions["FindNext"]->setEnabled(false);
	connect(m_documents, SIGNAL(findNextAvailable(bool)), m_actions["FindNext"], SLOT(setEnabled(bool)));
	m_actions["FindPrevious"] = tools_menu->addAction(QIcon::fromTheme("go-up"), tr("Find Pre&vious"), m_documents, SLOT(findPrevious()), QKeySequence::FindPrevious);
	m_actions["FindPrevious"]->setEnabled(false);
	connect(m_documents, SIGNAL(findNextAvailable(bool)), m_actions["FindPrevious"], SLOT(setEnabled(bool)));
	m_actions["Replace"] = tools_menu->addAction(QIcon::fromTheme("edit-find-replace"), tr("&Replace..."), m_documents, SLOT(replace()), keyBinding(QKeySequence::Replace, tr("Ctrl+R")));
	tools_menu->addSeparator();
	QMenu* quotes_menu = tools_menu->addMenu(tr("Smart &Quotes"));
	m_replace_document_quotes = quotes_menu->addAction(tr("Update &Document"), m_documents, SLOT(updateSmartQuotes()));
	m_replace_document_quotes->setStatusTip(tr("Update Document Smart Quotes"));
	ActionManager::instance()->addAction("SmartQuotesUpdateDocument", m_replace_document_quotes);
	m_replace_selection_quotes = quotes_menu->addAction(tr("Update &Selection"), m_documents, SLOT(updateSmartQuotesSelection()));
	m_replace_selection_quotes->setStatusTip(tr("Update Selection Smart Quotes"));
	ActionManager::instance()->addAction("SmartQuotesUpdateSelection", m_replace_selection_quotes);
	tools_menu->addSeparator();
	m_actions["CheckSpelling"] = tools_menu->addAction(QIcon::fromTheme("tools-check-spelling"), tr("&Spelling..."), m_documents, SLOT(checkSpelling()), tr("F7"));
	m_actions["SetDefaultLanguage"] = tools_menu->addAction(QIcon::fromTheme("accessories-dictionary"), tr("Set &Language..."), this, SLOT(setLanguageClicked()));
	tools_menu->addSeparator();
	m_actions["Timers"] = tools_menu->addAction(QIcon::fromTheme("appointment", QIcon::fromTheme("chronometer")), tr("&Timers..."), m_timers, SLOT(show()));
	m_actions["Symbols"] = tools_menu->addAction(QIcon::fromTheme("character-set"), tr("S&ymbols..."), m_documents, SLOT(showSymbols()));
	m_actions["DailyProgress"] = tools_menu->addAction(QIcon::fromTheme("view-calendar"), tr("&Daily Progress"), m_daily_progress_dialog, SLOT(show()));

	// Create settings menu
	QMenu* settings_menu = menuBar()->addMenu(tr("&Settings"));
	QAction* action = settings_menu->addAction(tr("Show &Toolbar"), this, SLOT(toggleToolbar(bool)));
	action->setCheckable(true);
	action->setChecked(QSettings().value("Toolbar/Shown", true).toBool());
	ActionManager::instance()->addAction("ShowToolbar", action);
#ifndef Q_OS_MAC
	action = settings_menu->addAction(tr("Show &Menu Icons"), this, SLOT(toggleMenuIcons(bool)));
	action->setCheckable(true);
	action->setChecked(QSettings().value("Window/MenuIcons", false).toBool());
	ActionManager::instance()->addAction("ShowMenuIcons", action);
#endif
	settings_menu->addSeparator();
	QMenu* focus_menu = settings_menu->addMenu(tr("F&ocused Text"));
	settings_menu->addSeparator();
	m_actions["Fullscreen"] = settings_menu->addAction(QIcon::fromTheme("view-fullscreen"), tr("&Fullscreen"), this, SLOT(toggleFullscreen()), tr("F11"));
#ifdef Q_OS_MAC
	m_actions["Fullscreen"]->setShortcut(tr("Esc"));
#else
	m_actions["Fullscreen"]->setCheckable(true);
#endif
	m_actions["Minimize"] = settings_menu->addAction(QIcon::fromTheme("arrow-down"), tr("M&inimize"), this, SLOT(minimize()), tr("Ctrl+M"));
	settings_menu->addSeparator();
	m_actions["Themes"] = settings_menu->addAction(QIcon::fromTheme("applications-graphics"), tr("&Themes..."), this, SLOT(themeClicked()));
	settings_menu->addSeparator();
	m_actions["PreferencesLocale"] = settings_menu->addAction(QIcon::fromTheme("preferences-desktop-locale"), tr("Application &Language..."), this, SLOT(setLocaleClicked()));
	m_actions["Preferences"] = settings_menu->addAction(QIcon::fromTheme("preferences-system"), tr("&Preferences..."), this, SLOT(preferencesClicked()), QKeySequence::Preferences);
	m_actions["Preferences"]->setMenuRole(QAction::PreferencesRole);

	// Create focus sub-menu
	QAction* focus_mode[4];
	focus_mode[0] = focus_menu->addAction(tr("&Off"));
	focus_mode[0]->setStatusTip(tr("Focus Off"));
	focus_mode[1] = focus_menu->addAction(tr("One &Line"));
	focus_mode[1]->setStatusTip(tr("Focus One Line"));
	focus_mode[2] = focus_menu->addAction(tr("&Three Lines"));
	focus_mode[2]->setStatusTip(tr("Focus Three Lines"));
	focus_mode[3] = focus_menu->addAction(tr("&Paragraph"));
	focus_mode[3]->setStatusTip(tr("Focus Paragraph"));
	m_focus_actions = new QActionGroup(this);
	for (int i = 0; i < 4; ++i) {
		focus_mode[i]->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + (Qt::Key_0 + i)));
		focus_mode[i]->setCheckable(true);
		focus_mode[i]->setData(i);
		m_focus_actions->addAction(focus_mode[i]);
	}
	focus_mode[0]->setShortcut(tr("Ctrl+Shift+`"));
	for (int i = 0; i < 4; ++i) {
		ActionManager::instance()->addAction(QString("FocusedText%1").arg(i), focus_mode[i]);
	}
	focus_mode[qBound(0, QSettings().value("Window/FocusedText").toInt(), 3)]->setChecked(true);
	connect(m_focus_actions, SIGNAL(triggered(QAction*)), m_documents, SLOT(setFocusMode(QAction*)));

	// Create help menu
	QMenu* help_menu = menuBar()->addMenu(tr("&Help"));
	m_actions["About"] = help_menu->addAction(QIcon::fromTheme("help-about"), tr("&About"), this, SLOT(aboutClicked()));
	m_actions["About"]->setMenuRole(QAction::AboutRole);
	m_actions["AboutQt"] = help_menu->addAction(
		QIcon(":/qt-project.org/qmessagebox/images/qtlogo-64.png"),
		tr("About &Qt"), qApp, SLOT(aboutQt()));
	m_actions["AboutQt"]->setMenuRole(QAction::AboutQtRole);

	// Always show menubar
#ifndef Q_OS_MAC
	connect(file_menu, SIGNAL(aboutToShow()), m_documents, SLOT(setHeaderVisible()));
	connect(edit_menu, SIGNAL(aboutToShow()), m_documents, SLOT(setHeaderVisible()));
	connect(format_menu, SIGNAL(aboutToShow()), m_documents, SLOT(setHeaderVisible()));
	connect(tools_menu, SIGNAL(aboutToShow()), m_documents, SLOT(setHeaderVisible()));
	connect(settings_menu, SIGNAL(aboutToShow()), m_documents, SLOT(setHeaderVisible()));
	connect(help_menu, SIGNAL(aboutToShow()), m_documents, SLOT(setHeaderVisible()));

	connect(file_menu, SIGNAL(aboutToHide()), m_documents, SLOT(showHeader()));
	connect(edit_menu, SIGNAL(aboutToHide()), m_documents, SLOT(showHeader()));
	connect(format_menu, SIGNAL(aboutToHide()), m_documents, SLOT(showHeader()));
	connect(tools_menu, SIGNAL(aboutToHide()), m_documents, SLOT(showHeader()));
	connect(settings_menu, SIGNAL(aboutToHide()), m_documents, SLOT(showHeader()));
	connect(help_menu, SIGNAL(aboutToHide()), m_documents, SLOT(showHeader()));
#endif

	// Enable toolbar management in preferences dialog
	QHashIterator<QString, QAction*> i(m_actions);
	while (i.hasNext()) {
		i.next();
		i.value()->setData(i.key());

		// Add to format actions
		if (i.key().startsWith("Format")) {
			m_format_actions.append(i.value());
		}

		// Load custom shortcut
		ActionManager::instance()->addAction(i.key(), i.value());
	}
	addActions(m_actions.values());
}

//-----------------------------------------------------------------------------
