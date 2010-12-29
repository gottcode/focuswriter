/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#include "session_manager.h"

#include "session.h"
#include "theme.h"
#include "window.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSettings>

#include <QtGui/QActionGroup>
#include <QtGui/QApplication>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QInputDialog>
#include <QtGui/QListWidget>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

//-----------------------------------------------------------------------------

SessionManager::SessionManager(Window* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_session(0),
	m_window(parent)
{
	setWindowTitle(tr("Manage Sessions"));

	// Create session lists
	m_sessions_menu = new QMenu(this);
	m_sessions_menu->setTitle(tr("S&essions"));
	m_sessions_actions = new QActionGroup(this);
	connect(m_sessions_actions, SIGNAL(triggered(QAction*)), this, SLOT(switchSession(QAction*)));

	m_sessions_list = new QListWidget(this);
	m_sessions_list->setIconSize(QSize(16,16));
	m_sessions_list->setMovement(QListWidget::Static);
	m_sessions_list->setResizeMode(QListWidget::Adjust);
	m_sessions_list->setSelectionMode(QListWidget::SingleSelection);
	connect(m_sessions_list, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(selectedSessionChanged(QListWidgetItem*)));
	connect(m_sessions_list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(switchSession()));

	// Create buttons
	QPushButton* new_button = new QPushButton(tr("New"), this);
	connect(new_button, SIGNAL(clicked()), this, SLOT(newSession()));

	m_rename_button = new QPushButton(tr("Rename"), this);
	connect(m_rename_button, SIGNAL(clicked()), this, SLOT(renameSession()));

	QPushButton* clone_button = new QPushButton(tr("Clone"), this);
	connect(clone_button, SIGNAL(clicked()), this, SLOT(cloneSession()));

	m_delete_button = new QPushButton(tr("Delete"), this);
	connect(m_delete_button, SIGNAL(clicked()), this, SLOT(deleteSession()));

	m_switch_button = new QPushButton(tr("Switch To"), this);
	connect(m_switch_button, SIGNAL(clicked()), this, SLOT(switchSession()));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	// Lay out window
	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnStretch(0, 1);
	layout->setRowStretch(5, 1);

	layout->addWidget(m_sessions_list, 0, 0, 6, 1);

	layout->addWidget(new_button, 0, 1);
	layout->addWidget(m_rename_button, 1, 1);
	layout->addWidget(clone_button, 2, 1);
	layout->addWidget(m_delete_button, 3, 1);
	layout->addWidget(m_switch_button, 4, 1);

	layout->addWidget(buttons, 7, 1);

	// Restore size
	resize(QSettings().value("SessionManager/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

SessionManager::~SessionManager()
{
	delete m_session;
	m_session = 0;
}

//-----------------------------------------------------------------------------

Session* SessionManager::current() const
{
	return m_session;
}

//-----------------------------------------------------------------------------

QMenu* SessionManager::menu() const
{
	return m_sessions_menu;
}

//-----------------------------------------------------------------------------

bool SessionManager::closeCurrent()
{
	if (m_session) {
		if (!m_window->closeDocuments(m_session->data())) {
			return false;
		}
		delete m_session;
		m_session = 0;
	}
	return true;
}

//-----------------------------------------------------------------------------

void SessionManager::setCurrent(const QString& session)
{
	// Close open documents
	if (!closeCurrent()) {
		return;
	}

	// Open session
	m_session = new Session(!session.isEmpty() ? session : Session::tr("Default"));
	emit themeChanged(m_session->theme());
	m_window->addDocuments(m_session->files(), m_session->positions(), m_session->active(), true);

	// Save session name
	if (!session.isEmpty()) {
		QSettings().setValue("SessionManager/Session", m_session->name());
	} else {
		QSettings().remove("SessionManager/Session");
	}

	updateList(session);
}

//-----------------------------------------------------------------------------

void SessionManager::newSession()
{
	// Fetch session name
	QString name = getSessionName(tr("New Session"));
	if (name.isEmpty()) {
		return;
	}
	QString theme = m_session->theme();

	// Close open documents
	bool visible = isVisible();
	hide();
	if (!closeCurrent()) {
		if (visible) {
			show();
		}
		return;
	}
	accept();

	// Create session and make it active
	{
		QSettings session(Session::pathFromName(name), QSettings::IniFormat);
		session.setValue("ThemeManager/Theme", theme);
		session.setValue("ThemeManager/Size", QSettings().value("ThemeManager/Size"));
	}
	setCurrent(name);
}

//-----------------------------------------------------------------------------

void SessionManager::hideEvent(QHideEvent* event)
{
	QSettings().setValue("SessionManager/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void SessionManager::cloneSession()
{
	// Find path
	QListWidgetItem* item = selectedSession(false);
	if (!item) {
		return;
	}
	QString filename = item != m_sessions_list->item(0) ? Session::pathFromName(item->text()) : "";

	// Fetch session name
	QString name = getSessionName(tr("Clone Session"));
	if (name.isEmpty()) {
		return;
	}

	// Close open documents
	hide();
	if (!closeCurrent()) {
		show();
		return;
	}
	accept();

	// Create session and make it active
	QSettings settings;
	if (!filename.isEmpty()) {
		QFile::copy(filename, Session::pathFromName(name));
	} else {
		QSettings session(Session::pathFromName(name), QSettings::IniFormat);
		session.setValue("ThemeManager/Theme", settings.value("ThemeManager/Theme"));
		session.setValue("ThemeManager/Size", settings.value("ThemeManager/Size"));
		session.setValue("Save/Current", settings.value("Save/Current"));
		if (settings.value("Save/RememberPositions", true).toBool()) {
			session.setValue("Save/Positions", settings.value("Save/Positions"));
		}
		session.setValue("Save/Active", settings.value("Save/Active"));
	}
	setCurrent(name);
}

//-----------------------------------------------------------------------------

void SessionManager::renameSession()
{
	QListWidgetItem* item = selectedSession(true);
	if (!item) {
		return;
	}

	// Fetch session name
	QString name = getSessionName(tr("Rename Session"), item->text());
	if (name.isEmpty()) {
		return;
	}

	// Rename session
	QString current = item->text();
	if (current == m_session->name()) {
		m_session->setName(name);
		QSettings().setValue("SessionManager/Session", m_session->name());
	} else {
		QFile::rename(Session::pathFromName(current), Session::pathFromName(name));
	}

	updateList(name);
}

//-----------------------------------------------------------------------------

void SessionManager::deleteSession()
{
	QListWidgetItem* item = selectedSession(true);
	if (!item) {
		return;
	}

	// Confirm removal
	if (QMessageBox::question(this, tr("Question"), tr("Delete selected session?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}

	// Delete session
	QString session = item->text();
	if (session == m_session->name()) {
		if (!closeCurrent()) {
			return;
		}
		setCurrent("");
	}
	QFile::remove(Session::pathFromName(session));

	updateList(m_session->name());
}

//-----------------------------------------------------------------------------

void SessionManager::switchSession()
{
	QListWidgetItem* item = selectedSession(false);
	if (item) {
		accept();
		setCurrent(item->data(Qt::UserRole).toString());
	}
}

//-----------------------------------------------------------------------------

void SessionManager::switchSession(QAction* session)
{
	setCurrent(session->data().toString());
}

//-----------------------------------------------------------------------------

void  SessionManager::selectedSessionChanged(QListWidgetItem* session)
{
	bool not_default = (session && session != m_sessions_list->item(0));
	if (not_default && m_session->name() == session->text()) {
		m_switch_button->setEnabled(false);
		m_rename_button->setEnabled(true);
		m_delete_button->setEnabled(true);
	} else {
		m_switch_button->setEnabled(session && m_session->name() != session->text());
		m_rename_button->setEnabled(not_default);
		m_delete_button->setEnabled(not_default);
	}
}

//-----------------------------------------------------------------------------

QListWidgetItem* SessionManager::selectedSession(bool prevent_default)
{
	QListWidgetItem* item = m_sessions_list->selectedItems().value(0, 0);
	if (!item || (prevent_default && item == m_sessions_list->item(0))) {
		return 0;
	} else {
		return item;
	}
}

//-----------------------------------------------------------------------------

QString SessionManager::getSessionName(const QString& title, const QString& session)
{
	QWidget* window = isVisible() ? this : parentWidget()->window();
	QString name = session;
	forever {
		bool ok;
		name = QInputDialog::getText(window, title, tr("Session name:"), QLineEdit::Normal, name, &ok);
		if (!ok) {
			return QString();
		}

		if (name != Session::tr("Default") && !QFile::exists(Session::pathFromName(name))) {
			break;
		} else {
			QMessageBox::information(window, tr("Sorry"), tr("The requested session name is already in use."));
		}
	}
	return name;
}

//-----------------------------------------------------------------------------

void SessionManager::updateList(const QString& selected)
{
	m_sessions_menu->clear();
	m_sessions_list->clear();

	QStringList files = QDir(Session::path(), "*.session").entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
	files.prepend(Session::tr("Default"));
	for (int i = 0; i < files.count(); ++i) {
		QString name = Session::pathToName(files.at(i));
		if ((name == Session::tr("Default")) && (i > 0)) {
			continue;
		}

		QAction* action = m_sessions_menu->addAction(name);
		action->setData(name);
		action->setCheckable(true);
		m_sessions_actions->addAction(action);
		QListWidgetItem* item = new QListWidgetItem(QIcon::fromTheme("folder"), name, m_sessions_list);
		item->setData(Qt::UserRole, name);

		if (name == m_session->name()) {
			action->setChecked(true);
			item->setIcon(QIcon::fromTheme("folder-open"));
		}

		if (name == selected) {
			m_sessions_list->setCurrentItem(item);
			m_sessions_list->scrollToItem(item);
		}
	}

	m_sessions_menu->addSeparator();
	m_sessions_menu->addAction(QIcon::fromTheme("window-new"), tr("&New..."), this, SLOT(newSession()), tr("Ctrl+Shift+N"));
	m_sessions_menu->addAction(QIcon::fromTheme("view-choose"), tr("&Manage..."), this, SLOT(exec()), tr("Ctrl+Shift+M"));
}

//-----------------------------------------------------------------------------
