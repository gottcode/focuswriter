/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "alert_layer.h"
#include "document.h"
#include "document_cache.h"
#include "load_screen.h"
#include "locale_dialog.h"
#include "preferences.h"
#include "preferences_dialog.h"
#include "session.h"
#include "session_manager.h"
#include "smart_quotes.h"
#include "sound.h"
#include "stack.h"
#include "theme.h"
#include "theme_manager.h"
#include "timer_display.h"
#include "timer_manager.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDate>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileOpenEvent>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSignalMapper>
#include <QStyle>
#include <QTabBar>
#include <QTextCodec>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QUrl>

//-----------------------------------------------------------------------------

extern bool compareFiles(const QString& filename1, const QString& filename2);

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

Window::Window(const QStringList& command_line_files)
	: m_toolbar(0),
	m_key_sound(0),
	m_enter_key_sound(0),
	m_fullscreen(true),
	m_auto_save(true),
	m_save_positions(true),
	m_goal_type(0),
	m_time_goal(0),
	m_wordcount_goal(0),
	m_current_time(0),
	m_current_wordcount(0)
{
	setAcceptDrops(true);
	setAttribute(Qt::WA_DeleteOnClose);
	setContextMenuPolicy(Qt::NoContextMenu);
	setCursor(Qt::WaitCursor);

	// Set up icons
	if (QIcon::themeName().isEmpty()) {
		QIcon::setThemeName("hicolor");
		setIconSize(QSize(22,22));
	}

	// Create window contents first so they stack behind documents
	menuBar();
	m_toolbar = new QToolBar(this);
	m_toolbar->setFloatable(false);
	m_toolbar->setMovable(false);
	addToolBar(m_toolbar);
	QWidget* contents = new QWidget(this);
	setCentralWidget(contents);

	// Set up thread for caching documents
	m_document_cache = new DocumentCache;
	m_document_cache_thread = new QThread(this);
	m_document_cache->moveToThread(m_document_cache_thread);
	m_document_cache_thread->start();

	// Create documents
	m_documents = new Stack(this);
	m_sessions = new SessionManager(this);
	m_timers = new TimerManager(m_documents, this);
	connect(m_documents, SIGNAL(footerVisible(bool)), m_timers->display(), SLOT(setVisible(bool)));
	connect(m_documents, SIGNAL(formattingEnabled(bool)), this, SLOT(setFormattingEnabled(bool)));
	connect(m_documents, SIGNAL(updateFormatActions()), this, SLOT(updateFormatActions()));
	connect(m_documents, SIGNAL(updateFormatAlignmentActions()), this, SLOT(updateFormatAlignmentActions()));
	connect(m_sessions, SIGNAL(themeChanged(Theme)), m_documents, SLOT(themeSelected(Theme)));

	contents->setMouseTracking(true);
	contents->installEventFilter(m_documents);

	// Set up menubar and toolbar
	initMenus();

	// Set up cache timer
	m_save_timer = new QTimer(this);
	m_save_timer->setInterval(600000);

	// Set up details
	m_footer = new QWidget(contents);
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

	// Set up tab navigation
	new QShortcut(QKeySequence::NextChild, this, SLOT(nextDocument()));
	new QShortcut(QKeySequence::PreviousChild, this, SLOT(previousDocument()));
	new QShortcut(Qt::CTRL + Qt::Key_0, this, SLOT(firstDocument()));
	new QShortcut(Qt::CTRL + Qt::Key_9, this, SLOT(lastDocument()));
	QSignalMapper* mapper = new QSignalMapper(this);
	for (int i = 1; i < 9 ; ++i) {
		QShortcut* shortcut = new QShortcut(Qt::CTRL + Qt::Key_0 + i, this);
		connect(shortcut, SIGNAL(activated()), mapper, SLOT(map()));
		mapper->setMapping(shortcut, i - 1);
	}
	connect(mapper, SIGNAL(mapped(int)), m_tabs, SLOT(setCurrentIndex(int)));

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
	QVBoxLayout* footer_layout = new QVBoxLayout(m_footer);
	footer_layout->setSpacing(0);
	footer_layout->setMargin(0);
	footer_layout->addWidget(details);
	footer_layout->addWidget(m_tabs);

	// Lay out window
	QVBoxLayout* layout = new QVBoxLayout(contents);
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addStretch();
	layout->addWidget(m_footer);

	// Load current daily progress
	QSettings settings;
	if (settings.value("Progress/Date").toDate() != QDate::currentDate()) {
		settings.remove("Progress");
	}
	settings.setValue("Progress/Date", QDate::currentDate().toString(Qt::ISODate));
	m_current_wordcount = settings.value("Progress/Words", 0).toInt();
	m_current_time = settings.value("Progress/Time", 0).toInt();
	updateProgress();

	// Restore window geometry
	setMinimumSize(640, 480);
	resize(800, 600);
	restoreGeometry(settings.value("Window/Geometry").toByteArray());
	show();
	m_fullscreen = !settings.value("Window/Fullscreen", true).toBool();
	toggleFullscreen();
	m_actions["Fullscreen"]->setChecked(m_fullscreen);

	// Load settings
	m_documents->loadScreen()->setText(tr("Loading settings"));
	Preferences preferences;
	loadPreferences(preferences);

	// Update and load theme
	m_documents->loadScreen()->setText(tr("Loading themes"));
	m_documents->themeSelected(settings.value("ThemeManager/Theme").toString());
	Theme::copyBackgrounds();

	// Update margin
	m_tabs->blockSignals(true);
	m_tabs->addTab(tr("Untitled"));
	updateMargin();
	m_tabs->removeTab(0);
	m_tabs->blockSignals(false);

	// Restore after crash
	bool writable = QFileInfo(Document::cachePath()).isWritable() && QFileInfo(Document::cachePath() + "/../").isWritable();
	if (!writable) {
		m_documents->alerts()->addAlert(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32,32), tr("Emergency cache is not writable."), QStringList());
	}
	QStringList files, datafiles;
	QString cachepath;
	QStringList entries = QDir(Document::cachePath()).entryList(QDir::Files);
	if (writable && (entries.count() > 1) && entries.contains("mapping")) {
		// Find cachedir
		QString date = QDate::currentDate().toString("yyyyMMdd");
		int extra = 0;
		QDir dir(QDir::cleanPath(Document::cachePath() + "/../"));
		QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
		foreach (const QString& subdir, subdirs) {
			if (subdir.startsWith(date)) {
				extra = qMax(extra, subdir.mid(9).toInt() + 1);
			}
		}
		cachepath = dir.absoluteFilePath(date + ((extra > 0) ? QString("-%1").arg(extra) : ""));

		// Move cache out of the way
		dir.rename("Files", cachepath);
		dir.mkdir("Files");

		// Read mapping of cached files
		QFile file(cachepath + "/mapping");
		if (file.open(QFile::ReadOnly | QFile::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			stream.setAutoDetectUnicode(true);

			while (!stream.atEnd()) {
				QString line = stream.readLine();
				QString datafile = line.section(' ', 0, 0);
				QString path = line.section(' ', 1);
				if (!datafile.isEmpty()) {
					files.append(path);
					datafiles.append(cachepath + "/" + datafile);
				}
			}
			file.close();
		}

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
			m_documents->loadScreen()->setText("");
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
		settings.setValue("Save/Current", command_line_files);
		settings.setValue("Save/Positions", QStringList());
		settings.setValue("Save/Active", 0);
	}
	m_sessions->setCurrent(session, files, datafiles);

	// Remove old cache
	if (!cachepath.isEmpty()) {
		QDir cachedir(cachepath);
		if ((cachedir.count() == 3) && (cachedir.entryList(QDir::Files).first() == "mapping")) {
			cachedir.remove("mapping");
			cachedir.rmdir(cachepath);
		}
	}

	// Bring to front
	activateWindow();
	raise();
	unsetCursor();

	m_save_timer->start();
}

//-----------------------------------------------------------------------------

Window::~Window()
{
	m_document_cache_thread->quit();
	m_document_cache_thread->wait();
	delete m_document_cache;
}

//-----------------------------------------------------------------------------

void Window::addDocuments(const QStringList& files, const QStringList& datafiles, const QStringList& positions, int active, bool show_load)
{
	m_documents->setHeaderVisible(false);
	m_documents->setFooterVisible(false);

	static const QStringList suffixes = QStringList()
		<< "abw" << "awt" << "zabw"
		<< "doc" << "dot"
		<< "docx" << "docm" << "dotx" << "dotm"
		<< "kwd"
		<< "ott"
		<< "wpd";
	QList<int> skip;
	for (int i = 0; i < files.count(); ++i) {
		if (suffixes.contains(QFileInfo(files.at(i)).suffix())) {
			skip += i;
		}
	}
	if (!skip.isEmpty()) {
		QStringList skipped;
		foreach (int i, skip) {
			skipped += QDir::toNativeSeparators(files.at(i));
		}
		QMessageBox mbox(window());
		mbox.setWindowTitle(tr("Sorry"));
		mbox.setText(tr("Some files are unsupported and will not be opened."));
		mbox.setDetailedText(skipped.join("\n"));
		mbox.setStandardButtons(QMessageBox::Ok);
		mbox.setDefaultButton(QMessageBox::Ok);
		mbox.setIcon(QMessageBox::Warning);
		mbox.exec();
	}

	show_load = show_load || ((files.count() > 1) && (files.count() > skip.count()) && !m_documents->loadScreen()->isVisible());
	if (show_load) {
		m_documents->loadScreen()->setText("");
		setCursor(Qt::WaitCursor);
	}

	int untitled_index = -1;
	int current_index = -1;
	if (m_documents->count()) {
		current_index = m_documents->currentIndex();
		Document* document = m_documents->count() ? m_documents->currentDocument() : 0;
		if (document->untitledIndex() && !document->text()->document()->isModified()) {
			untitled_index = m_documents->currentIndex();
		}
	}

	QStringList missing;
	QStringList readonly;
	int open_files = m_documents->count();
	for (int i = 0; i < files.count(); ++i) {
		if (!skip.isEmpty() && skip.first() == i) {
			skip.removeFirst();
			continue;
		} else if (!addDocument(files.at(i), datafiles.at(i), positions.value(i, "-1").toInt())) {
			missing.append(QDir::toNativeSeparators(files.at(i)));
		} else if (!files.at(i).isEmpty() && (m_documents->currentDocument()->untitledIndex() > 0)) {
			int index = m_documents->currentIndex();
			missing.append(QDir::toNativeSeparators(files.at(i)));
			m_documents->removeDocument(index);
			m_tabs->removeTab(index);
		} else if (m_documents->currentDocument()->isReadOnly() && m_documents->count() > open_files) {
			readonly.append(QDir::toNativeSeparators(files.at(i)));
		}
		open_files = m_documents->count();
	}
	if (m_documents->count() == 0) {
		newDocument();
	}

	if (untitled_index == -1) {
		if (active != -1) {
			m_tabs->setCurrentIndex(active);
		} else if (m_documents->currentIndex() == current_index) {
			m_tabs->setCurrentIndex(m_tabs->count() - 1);
		}
	} else {
		m_tabs->setCurrentIndex(untitled_index);
		closeDocument();
	}

	if (!missing.isEmpty()) {
		QMessageBox mbox(window());
		mbox.setWindowTitle(tr("Sorry"));
		mbox.setText(tr("Some files could not be opened."));
		mbox.setDetailedText(missing.join("\n"));
		mbox.setStandardButtons(QMessageBox::Ok);
		mbox.setDefaultButton(QMessageBox::Ok);
		mbox.setIcon(QMessageBox::Warning);
		mbox.exec();
	}
	if (!readonly.isEmpty()) {
		QMessageBox mbox(window());
		mbox.setWindowTitle(tr("Note"));
		mbox.setText(tr("Some files were opened Read-Only."));
		mbox.setDetailedText(readonly.join("\n"));
		mbox.setStandardButtons(QMessageBox::Ok);
		mbox.setDefaultButton(QMessageBox::Ok);
		mbox.setIcon(QMessageBox::Information);
		mbox.exec();
	}

	if (show_load) {
		m_documents->waitForThemeBackground();
		m_documents->loadScreen()->finish();
		unsetCursor();
	}
}

//-----------------------------------------------------------------------------

void Window::addDocuments(QDropEvent* event)
{
	if (event->mimeData()->hasUrls()) {
		QStringList files;
		foreach (QUrl url, event->mimeData()->urls()) {
			files.append(url.toLocalFile());
		}
		addDocuments(files, files);
		event->acceptProposedAction();
	}
}

//-----------------------------------------------------------------------------

bool Window::closeDocuments(QSettings* session)
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

	// Close files
	int count = m_documents->count();
	for (int i = 0; i < count; ++i) {
		m_documents->removeDocument(0);
		m_tabs->removeTab(0);
	}

	return true;
}

//-----------------------------------------------------------------------------

void Window::addDocuments(const QString& documents)
{
	QStringList files = documents.split(QLatin1String("\n"), QString::SkipEmptyParts);
	if (!files.isEmpty()) {
		addDocuments(files, files);
	}
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
	if (!m_timers->cancelEditing() || !m_sessions->closeCurrent()) {
		event->ignore();
		return;
	}
	QSettings().setValue("Window/FocusedText", m_focus_actions->checkedAction()->data().toInt());
	if (!m_fullscreen) {
		QSettings().setValue("Window/Geometry", saveGeometry());
	}
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
	static QString oldpath = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
	QString path = m_documents->currentDocument()->filename();
	if (!path.isEmpty()) {
		path = QFileInfo(path).dir().path();
	} else {
		path = oldpath;
	}

	QStringList filenames = QFileDialog::getOpenFileNames(window(), tr("Open File"), path, tr("Text Files (%1);;All Files (*)").arg("*.txt *.text *.odt *.rtf"));
	if (!filenames.isEmpty()) {
		addDocuments(filenames, filenames);
		oldpath = QFileInfo(filenames.last()).dir().path();
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
	if (m_documents->count() == 1) {
		newDocument();
	}
	m_documents->removeDocument(index);
	m_tabs->removeTab(index);
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

void Window::setFormattingEnabled(bool enabled)
{
	foreach (QAction* action, m_format_actions) {
		action->setEnabled(enabled);
	}
	if (enabled) {
		m_actions["FormatIndentDecrease"]->setEnabled(false);
		m_richtext_action->setVisible(false);
		m_plaintext_action->setVisible(true);
	} else {
		m_plaintext_action->setVisible(false);
		m_richtext_action->setVisible(true);
	}
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
	Preferences preferences;
	PreferencesDialog dialog(preferences, this);
	if (dialog.exec() == QDialog::Accepted) {
		loadPreferences(preferences);
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
			tr("Copyright &copy; 2008-%1 Graeme Gott").arg("2012"),
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

	if (document->text()->textCursor().blockFormat().layoutDirection() == Qt::LeftToRight) {
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
}

//-----------------------------------------------------------------------------

void Window::updateProgress()
{
	int progress = 0;
	if (m_goal_type == 1) {
		progress = (m_current_time * 100) / (m_time_goal * 60000);
	} else if (m_goal_type == 2) {
		progress = (m_current_wordcount * 100) / m_wordcount_goal;
	}
	m_progress_label->setText(tr("%1% of daily goal").arg(progress));
}

//-----------------------------------------------------------------------------

void Window::updateSave()
{
	m_actions["Save"]->setEnabled(m_documents->currentDocument()->text()->document()->isModified());
	m_actions["Rename"]->setDisabled(m_documents->currentDocument()->filename().isEmpty());
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
			if (m_documents->document(i)->filename() == canonical_filename) {
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
	show_load = !file.isEmpty() && !m_documents->loadScreen()->isVisible() && (info.size() > 100000);
	if (m_documents->loadScreen()->isVisible() || show_load) {
		if (!file.isEmpty()) {
			m_documents->loadScreen()->setText(tr("Opening %1").arg(file));
		} else {
			m_documents->loadScreen()->setText("");
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
	}
	Document* document = new Document(file, m_current_wordcount, m_current_time, this);
	m_documents->addDocument(document);
	document->loadTheme(m_sessions->current()->theme());
	document->setFocusMode(m_focus_actions->checkedAction()->data().toInt());
	if (document->loadFile(path, m_save_positions ? position : -1)) {
		if (datafile != file) {
			document->text()->document()->setModified(!compareFiles(file, datafile));
			QFile::remove(datafile);
		}
	} else {
		document->loadFile(file, m_save_positions ? position : -1);
	}
	connect(document, SIGNAL(changed()), this, SLOT(updateDetails()));
	connect(document, SIGNAL(changed()), this, SLOT(updateProgress()));
	connect(document, SIGNAL(changedName()), this, SLOT(updateSave()));
	connect(document, SIGNAL(indentChanged(bool)), m_actions["FormatIndentDecrease"], SLOT(setEnabled(bool)));
	connect(document->text()->document(), SIGNAL(modificationChanged(bool)), this, SLOT(updateSave()));
	connect(document, SIGNAL(cacheFile(DocumentWriter*)), m_document_cache, SLOT(cacheFile(DocumentWriter*)));
	connect(document, SIGNAL(removeCacheFile(QString)), m_document_cache, SLOT(removeCacheFile(QString)));

	// Add tab for document
	int index = m_tabs->addTab(tr("Untitled"));
	updateTab(index);
	m_tabs->setCurrentIndex(index);

	if (show_load) {
		m_documents->loadScreen()->finish();
	}

	return true;
}

//-----------------------------------------------------------------------------

bool Window::saveDocument(int index)
{
	Document* document = m_documents->document(index);
	if (!document->text()->document()->isModified()) {
		return true;
	}

	// Auto-save document
	if (m_auto_save && document->text()->document()->isModified() && !document->filename().isEmpty()) {
		return document->save();
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

//-----------------------------------------------------------------------------

void Window::loadPreferences(Preferences& preferences)
{
	if (preferences.typewriterSounds() && (!m_key_sound || !m_enter_key_sound)) {
		if (m_documents->loadScreen()->isVisible()) {
			m_documents->loadScreen()->setText(tr("Loading sounds"));
		}
		m_key_sound = new Sound(Qt::Key_Any, "keyany.wav", this);
		m_enter_key_sound = new Sound(Qt::Key_Enter, "keyenter.wav", this);

		if (!m_key_sound->isValid() || !m_enter_key_sound->isValid()) {
			m_documents->alerts()->addAlert(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32,32), tr("Unable to load typewriter sounds."), QStringList());
			delete m_key_sound;
			delete m_enter_key_sound;
			m_key_sound = m_enter_key_sound = 0;
			preferences.setTypewriterSounds(false);
		}
	}
	Sound::setEnabled(preferences.typewriterSounds());

	m_auto_save = preferences.autoSave();
	if (m_auto_save) {
		disconnect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoCache()));
		connect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoSave()));
	} else {
		disconnect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoSave()));
		connect(m_save_timer, SIGNAL(timeout()), m_documents, SLOT(autoCache()));
	}
	m_save_positions = preferences.savePositions();

	SmartQuotes::loadPreferences(preferences);

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

	m_replace_document_quotes->setEnabled(preferences.smartQuotes());
	m_replace_selection_quotes->setEnabled(preferences.smartQuotes());
}

//-----------------------------------------------------------------------------

void Window::hideInterface()
{
	m_documents->setFooterVisible(false);
	m_documents->setHeaderVisible(false);
	for (int i = 0; i < m_documents->count(); ++i) {
		m_documents->document(i)->setScrollBarVisible(false);
	}
}

//-----------------------------------------------------------------------------

void Window::updateMargin()
{
	int header = centralWidget()->mapToParent(QPoint(0,0)).y();
	int footer = m_footer->sizeHint().height();
	m_documents->setMargins(footer, header);
}

//-----------------------------------------------------------------------------

void Window::updateTab(int index)
{
	Document* document = m_documents->document(index);
	QString filename = document->filename();
	QString name = QFileInfo(filename).fileName();
	if (name.isEmpty()) {
		name = tr("(Untitled %1)").arg(document->untitledIndex());
	}
	if (document->isReadOnly()) {
		name = tr("%1 (Read-Only)").arg(name);
	}
	bool modified = document->text()->document()->isModified();
	m_tabs->setTabText(index, name + (modified ? "*" : ""));
	m_tabs->setTabToolTip(index, QDir::toNativeSeparators(filename));
	if (document == m_documents->currentDocument()) {
		setWindowFilePath(name);
		setWindowModified(modified);
		updateWriteState(index);
	}
}

//-----------------------------------------------------------------------------

void Window::updateWriteState(int index)
{
	Document* document = m_documents->document(index);

	bool writable = !document->isReadOnly();
	m_actions["Paste"]->setEnabled(writable);
	m_actions["Replace"]->setEnabled(writable);
	m_actions["CheckSpelling"]->setEnabled(writable);
	if (writable) {
		connect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Cut"], SLOT(setEnabled(bool)));
	} else {
		disconnect(m_documents, SIGNAL(copyAvailable(bool)), m_actions["Cut"], SLOT(setEnabled(bool)));
		m_actions["Cut"]->setEnabled(false);
	}
	m_richtext_action->setEnabled(writable);
	m_replace_document_quotes->setEnabled(writable);
	m_replace_selection_quotes->setEnabled(writable);

	writable &= document->isRichText();
	m_plaintext_action->setEnabled(writable);
	foreach (QAction* action, m_format_actions) {
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

	format_menu->addSeparator();
	m_plaintext_action = format_menu->addAction(tr("&Make Plain Text"), m_documents, SLOT(makePlainText()));
	m_richtext_action = format_menu->addAction(tr("&Make Rich Text"), m_documents, SLOT(makeRichText()));
	m_richtext_action->setVisible(false);

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
	m_replace_selection_quotes = quotes_menu->addAction(tr("Update &Selection"), m_documents, SLOT(updateSmartQuotesSelection()));
	tools_menu->addSeparator();
	m_actions["CheckSpelling"] = tools_menu->addAction(QIcon::fromTheme("tools-check-spelling"), tr("&Spelling..."), m_documents, SLOT(checkSpelling()), tr("F7"));
	m_actions["Timers"] = tools_menu->addAction(QIcon::fromTheme("appointment", QIcon::fromTheme("chronometer")), tr("&Timers..."), m_timers, SLOT(show()));
	m_actions["Symbols"] = tools_menu->addAction(QIcon::fromTheme("character-set"), tr("S&ymbols..."), m_documents, SLOT(showSymbols()));

	// Create settings menu
	QMenu* settings_menu = menuBar()->addMenu(tr("&Settings"));
	QAction* action = settings_menu->addAction(tr("Show &Toolbar"), this, SLOT(toggleToolbar(bool)));
	action->setCheckable(true);
	action->setChecked(QSettings().value("Toolbar/Shown", true).toBool());
#ifndef Q_OS_MAC
	action = settings_menu->addAction(tr("Show &Menu Icons"), this, SLOT(toggleMenuIcons(bool)));
	action->setCheckable(true);
	action->setChecked(QSettings().value("Window/MenuIcons", false).toBool());
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
	focus_mode[1] = focus_menu->addAction(tr("One &Line"));
	focus_mode[2] = focus_menu->addAction(tr("&Three Lines"));
	focus_mode[3] = focus_menu->addAction(tr("&Paragraph"));
	m_focus_actions = new QActionGroup(this);
	for (int i = 0; i < 4; ++i) {
		focus_mode[i]->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + (Qt::Key_0 + i)));
		focus_mode[i]->setCheckable(true);
		focus_mode[i]->setData(i);
		m_focus_actions->addAction(focus_mode[i]);
	}
	focus_mode[qBound(0, QSettings().value("Window/FocusedText").toInt(), 3)]->setChecked(true);
	connect(m_focus_actions, SIGNAL(triggered(QAction*)), m_documents, SLOT(setFocusMode(QAction*)));

	// Create help menu
	QMenu* help_menu = menuBar()->addMenu(tr("&Help"));
	m_actions["About"] = help_menu->addAction(QIcon::fromTheme("help-about"), tr("&About"), this, SLOT(aboutClicked()));
	m_actions["About"]->setMenuRole(QAction::AboutRole);
	m_actions["AboutQt"] = help_menu->addAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), qApp, SLOT(aboutQt()));
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
	}
	addActions(m_actions.values());
}

//-----------------------------------------------------------------------------
