/*
	SPDX-FileCopyrightText: 2013-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "dictionary_dialog.h"

#include "dictionary_manager.h"
#include "locale_dialog.h"
#include "preferences.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

DictionaryDialog::DictionaryDialog(QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
	setWindowTitle(tr("Set Language"));

	m_languages = new QListWidget(this);
	const QStringList languages = DictionaryManager::instance().availableDictionaries();
	const QString current_language = Preferences::instance().language();
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
