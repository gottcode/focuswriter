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

QString LocaleDialog::m_current;
QString LocaleDialog::m_path;

//-----------------------------------------------------------------------------

LocaleDialog::LocaleDialog(QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(QCoreApplication::applicationName());

	QLabel* text = new QLabel(tr("Select application language:"), this);

	m_translations = new QComboBox(this);
	QStringList translations = findTranslations();
	QHash<QString, QString> display_texts;
	display_texts.insert("cs", QString::fromUtf8("\304\214esky (cs)"));
	display_texts.insert("de", QLatin1String("Deutsch (de)"));
	display_texts.insert("en_US", QLatin1String("English (en_US)"));
	display_texts.insert("es", QString::fromUtf8("Espa\303\261ol (es)"));
	display_texts.insert("es_MX", QString::fromUtf8("Espa\303\261ol (es_MX)"));
	display_texts.insert("fr", QString::fromUtf8("Fran\303\247ais (fr)"));
	display_texts.insert("pl", QLatin1String("Polski (pl)"));
	display_texts.insert("pt", QString::fromUtf8("Portugu\303\252s (pt)"));
	display_texts.insert("pt_BR", QString::fromUtf8("Portugu\303\252s (pt_BR)"));
	display_texts.insert("ru", QString::fromUtf8("\320\240\321\203\321\201\321\201\320\272\320\270\320\271 (ru)"));
	foreach (const QString& translation, translations) {
		if (translation.startsWith("qt")) {
			continue;
		}
		QString display = display_texts.value(translation);
		if (display.isEmpty()) {
			QLocale locale(translation);
			display = QString("%1 (%2)").arg(QLocale::languageToString(locale.language())).arg(translation);
		}
		m_translations->addItem(display, translation);
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

void LocaleDialog::loadTranslator()
{
	QString appdir = QCoreApplication::applicationDirPath();

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
	m_current = QSettings().value("Locale/Language", QLocale::system().name()).toString();
	QStringList translations = findTranslations();
	if (!translations.contains(m_current)) {
		m_current = m_current.left(2);
		if (!translations.contains(m_current)) {
			m_current = "en_US";
		}
	}
	QLocale::setDefault(m_current);

	// Load translators
	static QTranslator qt_translator;
	if (translations.contains("qt_" + m_current) || translations.contains("qt_" + m_current.left(2))) {
		qt_translator.load("qt_" + m_current, m_path);
	} else {
		qt_translator.load("qt_" + m_current, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	}
	QCoreApplication::installTranslator(&qt_translator);

	static QTranslator translator;
	translator.load(m_current, m_path);
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
