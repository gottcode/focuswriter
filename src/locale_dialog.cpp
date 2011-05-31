/***********************************************************************
 *
 * Copyright (C) 2010, 2011 Graeme Gott <graeme@gottcode.org>
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
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QLabel>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QTranslator>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

namespace
{
	class LocaleNames
	{
		LocaleNames()
		{
			m_names["ca"] = QString::fromUtf8("Catal\303\240");
			m_names["cs"] = QString::fromUtf8("\304\214esky");
			m_names["de"] = QLatin1String("Deutsch");
			m_names["en"] = QLatin1String("English");
			m_names["es"] = QString::fromUtf8("Espa\303\261ol");
			m_names["es_MX"] = QString::fromUtf8("Espa\303\261ol (M\303\251xico)");
			m_names["fi"] = QLatin1String("Suomi");
			m_names["fr"] = QString::fromUtf8("Fran\303\247ais");
			m_names["he"] = QString::fromUtf8("\327\242\326\264\327\221\326\260\327\250\326\264\327\231\327\252");
			m_names["it"] = QLatin1String("Italiano");
			m_names["pl"] = QLatin1String("Polski");
			m_names["pt"] = QString::fromUtf8("Portugu\303\252s");
			m_names["pt_BR"] = QString::fromUtf8("Portugu\303\252s (Brasil)");
			m_names["ru"] = QString::fromUtf8("\320\240\321\203\321\201\321\201\320\272\320\270\320\271");
			m_names["uk"] = QString::fromUtf8("\320\243\320\272\321\200\320\260\321\227\320\275\321\201\321\214\320\272\320\260");
			m_names["uk_UA"] = QString::fromUtf8("\320\243\320\272\321\200\320\260\321\227\320\275\321\201\321\214\320\272\320\260 (\320\243\320\272\321\200\320\260\321\227\320\275\320\260)");
		}

		QHash<QString, QString> m_names;

	public:
		static QString toString(const QString& name)
		{
			static LocaleNames locale_names;
			QString locale_name = locale_names.m_names.value(name);
			if (locale_name.isEmpty()) {
				QLocale locale(name);
				QString language = QLocale::languageToString(locale.language());
				if (locale.country() != QLocale::AnyCountry) {
					QString country = QLocale::countryToString(locale.country());
					locale_name = QString("%1 (%2)").arg(language, country);
				} else {
					locale_name = language;
				}
				locale_names.m_names[name] = locale_name;
			}
			return locale_name;
		}
	};
}

//-----------------------------------------------------------------------------

QString LocaleDialog::m_current;
QString LocaleDialog::m_path;
QString LocaleDialog::m_appname;

//-----------------------------------------------------------------------------

LocaleDialog::LocaleDialog(QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(QCoreApplication::applicationName());

	QLabel* text = new QLabel(tr("Select application language:"), this);

	m_translations = new QComboBox(this);
	m_translations->addItem(tr("<System Language>"));
	QString translation;
	QStringList translations = findTranslations();
	foreach (translation, translations) {
		if (translation.startsWith("qt")) {
			continue;
		}
		translation.remove(m_appname);
		m_translations->addItem(LocaleNames::toString(translation), translation);
	}
	int index = qMax(0, m_translations->findData(m_current));
	m_translations->setCurrentIndex(index);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSizeConstraint(QLayout::SetFixedSize);
	layout->addWidget(text);
	layout->addWidget(m_translations);
	layout->addWidget(buttons);
}

//-----------------------------------------------------------------------------

void LocaleDialog::loadTranslator(const QString& name)
{
	QString appdir = QCoreApplication::applicationDirPath();
	m_appname = name;

	// Find translator path
	QStringList paths;
	paths.append(appdir + "/translations/");
	paths.append(appdir + "/../share/" + QCoreApplication::applicationName().toLower() + "/translations/");
	paths.append(appdir + "/../Resources/translations");
	foreach (const QString& path, paths) {
		if (QFile::exists(path)) {
			m_path = path;
			break;
		}
	}

	// Find current locale
	m_current = QSettings().value("Locale/Language").toString();
	QString current = !m_current.isEmpty() ? m_current : QLocale::system().name();
	QStringList translations = findTranslations();
	if (!translations.contains(m_appname + current)) {
		current = current.left(2);
		if (!translations.contains(m_appname + current)) {
			current.clear();
		}
	}
	if (!current.isEmpty()) {
		QLocale::setDefault(m_appname + current);
	} else {
		current = "en";
	}

	// Load translators
	static QTranslator qt_translator;
	if (translations.contains("qt_" + current) || translations.contains("qt_" + current.left(2))) {
		qt_translator.load("qt_" + current, m_path);
	} else {
		qt_translator.load("qt_" + current, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	}
	QCoreApplication::installTranslator(&qt_translator);

	static QTranslator translator;
	translator.load(m_appname + current, m_path);
	QCoreApplication::installTranslator(&translator);
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
