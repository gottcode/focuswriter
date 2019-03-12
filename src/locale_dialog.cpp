/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2014, 2015, 2016, 2018, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "locale_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QLabel>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QTranslator>
#include <QVBoxLayout>

#include <algorithm>

//-----------------------------------------------------------------------------

QString LocaleDialog::m_current;
QString LocaleDialog::m_path;
QString LocaleDialog::m_appname;

//-----------------------------------------------------------------------------

LocaleDialog::LocaleDialog(QWidget* parent) :
	QDialog(parent, Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
	QString title = parent ? parent->window()->windowTitle() : QString();
	setWindowTitle(!title.isEmpty() ? title : QCoreApplication::applicationName());

	QLabel* text = new QLabel(tr("Select application language:"), this);

	m_translations = new QComboBox(this);
	m_translations->addItem(tr("<System Language>"));
	QStringList translations = findTranslations();
	for (QString translation : translations) {
		if (translation.startsWith("qt")) {
			continue;
		}
		translation.remove(m_appname);
		m_translations->addItem(languageName(translation), translation);
	}
	int index = std::max(0, m_translations->findData(m_current));
	m_translations->setCurrentIndex(index);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &LocaleDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &LocaleDialog::reject);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSizeConstraint(QLayout::SetFixedSize);
	layout->addWidget(text);
	layout->addWidget(m_translations);
	layout->addWidget(buttons);
}

//-----------------------------------------------------------------------------

void LocaleDialog::loadTranslator(const QString& name, const QStringList& datadirs)
{
	m_appname = name;

	// Find translator path
	QStringList paths = datadirs;
	if (paths.isEmpty()) {
		QString appdir = QCoreApplication::applicationDirPath();
		paths.append(appdir);
		paths.append(appdir + "/../share/" + QCoreApplication::applicationName().toLower());
		paths.append(appdir + "/../Resources");
	}
	for (const QString& path : paths) {
		if (QFile::exists(path + "/translations/")) {
			m_path = path + "/translations/";
			break;
		}
	}

	// Find current locale
	m_current = QSettings().value("Locale/Language").toString();
	if (!m_current.isEmpty()) {
		QLocale::setDefault(m_current);
	}
	const QLocale locale;

	// Load translators
	static QTranslator translator;
	if (translator.load(locale, m_appname, "", m_path)) {
		QCoreApplication::installTranslator(&translator);

		const QString path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

		static QTranslator qtbase_translator;
		if (qtbase_translator.load(locale, "qtbase", "_", m_path) || qtbase_translator.load(locale, "qtbase", "_", path)) {
			QCoreApplication::installTranslator(&qtbase_translator);
		}

		static QTranslator qt_translator;
		if (qt_translator.load(locale, "qt", "_", m_path) || qt_translator.load(locale, "qt", "_", path)) {
			QCoreApplication::installTranslator(&qt_translator);
		}
	}
}

//-----------------------------------------------------------------------------

QString LocaleDialog::languageName(const QString& language)
{
	QString name;
	const QLocale locale(language);
	if (language.contains('_')) {
		if (locale.name() == language) {
			name = locale.nativeLanguageName() + " (" + locale.nativeCountryName() + ")";
		} else {
			name = locale.nativeLanguageName() + " (" + language + ")";
		}
	} else {
		name = locale.nativeLanguageName();
	}
	if (name.isEmpty() || name == "C") {
		if (language == "eo") {
			name = "Esperanto";
		} else {
			name = language;
		}
	}
	if (locale.textDirection() == Qt::RightToLeft) {
		name.prepend(QChar(0x202b));
	}
	return name;
}

//-----------------------------------------------------------------------------

QStringList LocaleDialog::findTranslations()
{
	QStringList result = QDir(m_path, "*.qm").entryList(QDir::Files);
	result.replaceInStrings(".qm", "");
	return result;
}

//-----------------------------------------------------------------------------

void LocaleDialog::accept()
{
	int current = m_translations->findData(m_current);
	if (current == m_translations->currentIndex()) {
		return reject();
	}
	QDialog::accept();

	m_current = m_translations->itemData(m_translations->currentIndex()).toString();
	QSettings().setValue("Locale/Language", m_current);
	QMessageBox::information(this, tr("Note"), tr("Please restart this application for the change in language to take effect."), QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
