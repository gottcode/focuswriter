/***********************************************************************
 *
 * Copyright (C) 2013, 2014, 2019 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_dialog.h"

#include "dictionary_manager.h"
#include "locale_dialog.h"
#include "preferences.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

DictionaryDialog::DictionaryDialog(QWidget* parent) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(tr("Set Language"));

	m_languages = new QListWidget(this);
	QStringList languages = DictionaryManager::instance().availableDictionaries();
	QString current_language = Preferences::instance().language();
	for (const QString& language : languages) {
		QListWidgetItem* item = new QListWidgetItem(LocaleDialog::languageName(language), m_languages);
		item->setData(Qt::UserRole, language);
		if (language == current_language) {
			m_languages->setCurrentItem(item);
		}
	}
	m_languages->sortItems(Qt::AscendingOrder);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &DictionaryDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &DictionaryDialog::reject);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_languages, 1);
	layout->addSpacing(layout->contentsMargins().top());
	layout->addWidget(buttons);
}

//-----------------------------------------------------------------------------

void DictionaryDialog::accept()
{
	if (m_languages->count() > 0) {
		Preferences::instance().setLanguage(m_languages->currentItem()->data(Qt::UserRole).toString());
		Preferences::instance().saveChanges();
	}
	QDialog::accept();
}

//-----------------------------------------------------------------------------
