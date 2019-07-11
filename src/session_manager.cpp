/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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
#include "utils.h"
#include "window.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#include <QActionGroup>
#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QInputDialog>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

#include <algorithm>

//-----------------------------------------------------------------------------

SessionManager::SessionManager(Window* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint),
	m_session(0),
	m_window(parent)
{
	setWindowTitle(tr("Manage Sessions"));

	// Rename session files
	QSettings settings;
	QString selected = settings.value("SessionManager/Session").toString();
	if (selected == Session::tr("Default")) {
		selected.clear();
		settings.remove("SessionManager/Session");
	}

	QDir dir(Session::path(), "*.session");
	QStringList files = dir.entryList(QDir::Files);
	for (const QString& file : files) {
		QString path = dir.filePath(file);
		QString name = QSettings(path, QSettings::IniFormat).value("Name").toString();
		if (!name.isEmpty()) {
			continue;
		}

		QString id = Session::createId();
		name = QUrl::fromPercentEncoding(QFileInfo(path).completeBaseName().toUtf8());
		QSettings(path, QSettings::IniFormat).setValue("Name", name);
		dir.rename(file, id + ".session");

		if (name == selected) {
			settings.setValue("SessionManager/Session", id);
		}
	}

	// Create session lists
	m_sessions_menu = new QMenu(this);
	m_sessions_menu->setTitle(tr("S&essions"));
	m_sessions_actions = new QActionGroup(this);
	connect(m_sessions_actions, &QActionGroup::triggered, this, QOverload<QAction*>::of(&SessionManager::switchSession));

	m_sessions_list = new QListWidget(this);
	m_sessions_list->setIconSize(QSize(16,16));
	m_sessions_list->setMovement(QListWidget::Static);
	m_sessions_list->setResizeMode(QListWidget::Adjust);
	m_sessions_list->setSelectionMode(QListWidget::SingleSelection);
	m_sessions_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	connect(m_sessions_list, &QListWidget::currentItemChanged, this, &SessionManager::selectedSessionChanged);
	connect(m_sessions_list, &QListWidget::itemActivated, this, QOverload<>::of(&SessionManager::switchSession));

	// Create buttons
	QPushButton* new_button = new QPushButton(tr("New"), this);
	connect(new_button, &QPushButton::clicked, this, &SessionManager::newSession);

	QPushButton* clone_button = new QPushButton(tr("Duplicate"), this);
	connect(clone_button, &QPushButton::clicked, this, &SessionManager::cloneSession);

	m_rename_button = new QPushButton(tr("Rename"), this);
	connect(m_rename_button, &QPushButton::clicked, this, &SessionManager::renameSession);

	m_delete_button = new QPushButton(tr("Delete"), this);
	connect(m_delete_button, &QPushButton::clicked, this, &SessionManager::deleteSession);

	m_switch_button = new QPushButton(tr("Switch To"), this);
	connect(m_switch_button, &QPushButton::clicked, this, QOverload<>::of(&SessionManager::switchSession));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &SessionManager::reject);

	// Lay out window
	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnStretch(0, 1);
	layout->setRowStretch(5, 1);

	layout->addWidget(m_sessions_list, 0, 0, 6, 1);

	layout->addWidget(new_button, 0, 1);
	layout->addWidget(clone_button, 1, 1);
	layout->addWidget(m_rename_button, 2, 1);
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

bool SessionManager::saveCurrent()
{
	return m_session ? m_window->saveDocuments(m_session->data()) : true;
}

//-----------------------------------------------------------------------------

void SessionManager::setCurrent(const QString& id, const QStringList& files, const QStringList& datafiles)
{
	// Close open documents
	if (!closeCurrent()) {
		updateList(m_session->id());
		return;
	}

	// Open session
	m_session = new Session(id);

	QString theme = m_session->theme();
	bool is_default = m_session->themeDefault();
	if (!QFile::exists(Theme::filePath(theme, is_default))) {
		theme = Theme::defaultId();
		is_default = true;
		m_session->setTheme(theme, is_default);
	}
	emit themeChanged(Theme(theme, is_default));

	if (files.isEmpty()) {
		m_window->addDocuments(m_session->files(), m_session->files(), m_session->positions(), m_session->active(), true);
	} else {
		m_window->addDocuments(files, datafiles, m_session->positions(), m_session->active(), true);
	}

	// Save session name
	if (!m_session->id().isEmpty()) {
		QSettings().setValue("SessionManager/Session", m_session->id());
	} else {
		QSettings().remove("SessionManager/Session");
	}

	updateList(m_session->id());
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
	bool is_default = m_session->themeDefault();

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
	QString id = Session::createId();
	{
		QSettings session(Session::pathFromId(id), QSettings::IniFormat);
		session.setValue("Name", name);
		session.setValue("ThemeManager/Theme", theme);
		session.setValue("ThemeManager/ThemeDefault", is_default);
		session.setValue("ThemeManager/Size", QSettings().value("ThemeManager/Size"));
	}
	setCurrent(id);
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
	QString id = item->data(Qt::UserRole).toString();
	QString filename = item != m_sessions_list->item(0) ? Session::pathFromId(id) : "";

	// Fetch session name
	QStringList values = splitStringAtLastNumber(item->text());
	int count = values.at(1).toInt();
	QString name;
	do {
		++count;
		name = values.at(0) + QString::number(count);
	} while (Session::exists(name));

	name = getSessionName(tr("Duplicate Session"), name);
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
	id = Session::createId();
	QString path = Session::pathFromId(id);
	if (!filename.isEmpty()) {
		QFile::copy(filename, path);
		QSettings(path, QSettings::IniFormat).setValue("Name", name);
	} else {
		QSettings settings;
		QSettings session(path, QSettings::IniFormat);
		session.setValue("Name", name);
		session.setValue("ThemeManager/Theme", settings.value("ThemeManager/Theme"));
		session.setValue("ThemeManager/ThemeDefault", settings.value("ThemeManager/ThemeDefault", false));
		session.setValue("ThemeManager/Size", settings.value("ThemeManager/Size"));
		session.setValue("Save/Current", settings.value("Save/Current"));
		if (settings.value("Save/RememberPositions", true).toBool()) {
			session.setValue("Save/Positions", settings.value("Save/Positions"));
		}
		session.setValue("Save/Active", settings.value("Save/Active"));
	}
	setCurrent(id);
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
	QString id = item->data(Qt::UserRole).toString();
	if (id == m_session->id()) {
		m_session->setName(name);
		QSettings().setValue("SessionManager/Session", m_session->id());
	} else {
		QSettings(Session::pathFromId(id), QSettings::IniFormat).setValue("Name", name);
	}

	updateList(id);
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
	QString id = item->data(Qt::UserRole).toString();
	if (id == m_session->id()) {
		if (!closeCurrent()) {
			return;
		}
		setCurrent("");
	}
	QFile::remove(Session::pathFromId(id));

	updateList(m_session->id());
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
	QString id = session ? session->data(Qt::UserRole).toString() : "";
	if (not_default && m_session->id() == id) {
		m_switch_button->setEnabled(false);
		m_rename_button->setEnabled(true);
		m_delete_button->setEnabled(true);
	} else {
		m_switch_button->setEnabled(session && m_session->id() != id);
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
		name = QInputDialog::getText(window, title, tr("Session name:"), QLineEdit::Normal, name, &ok).simplified();
		if (!ok) {
			return QString();
		}

		if (!name.isEmpty() && name != Session::tr("Default") && !Session::exists(name)) {
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

	QDir dir(Session::path(), "*.session");
	QStringList files = dir.entryList(QDir::Files);

	QHash<QString, QString> ids;
	for (const QString& file : files) {
		QString id = QFileInfo(file).completeBaseName();
		QString name = QSettings(dir.filePath(file), QSettings::IniFormat).value("Name").toString();
		if (name == Session::tr("Default")) {
			continue;
		}
		ids.insert(name, id);
	}

	files = ids.keys();
	std::sort(files.begin(), files.end(), localeAwareSort);

	ids.insert(Session::tr("Default"), QString());
	files.prepend(Session::tr("Default"));

	for (const QString& name : files) {
		QString id = ids.value(name);

		QAction* action = m_sessions_menu->addAction(fontMetrics().elidedText(name, Qt::ElideRight, 3 * logicalDpiX()));
		action->setData(id);
		action->setCheckable(true);
		m_sessions_actions->addAction(action);

		QListWidgetItem* item = new QListWidgetItem(QIcon::fromTheme("folder"), name, m_sessions_list);
		item->setToolTip(name);
		item->setData(Qt::UserRole, id);

		if (id == m_session->id()) {
			action->setChecked(true);
			item->setIcon(QIcon::fromTheme("folder-open"));
		}

		if (id == selected) {
			m_sessions_list->setCurrentItem(item);
			m_sessions_list->scrollToItem(item);
		}
	}

	m_sessions_menu->addSeparator();
	m_sessions_menu->addAction(QIcon::fromTheme("window-new"), tr("&New..."), this, &SessionManager::newSession, tr("Ctrl+Shift+N"));
	m_sessions_menu->addAction(QIcon::fromTheme("view-choose"), tr("&Manage..."), this, &SessionManager::exec, tr("Ctrl+Shift+M"));
}

//-----------------------------------------------------------------------------
