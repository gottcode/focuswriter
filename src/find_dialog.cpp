/***********************************************************************
 *
 * Copyright (C) 2008-2009 Graeme Gott <graeme@gottcode.org>
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

#include "find_dialog.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>

/*****************************************************************************/

FindDialog::FindDialog(QPlainTextEdit* document, QWidget* parent)
: QDialog(parent),
  m_document(document) {
	setWindowTitle(tr("Find"));

	// Create line edits
	m_find_string = new QLineEdit(this);
	m_replace_string = new QLineEdit(this);

	QGridLayout* edit_layout = new QGridLayout;
	edit_layout->addWidget(new QLabel(tr("Search for:"), this), 0, 0);
	edit_layout->addWidget(m_find_string, 0, 1);
	edit_layout->addWidget(new QLabel(tr("Replace with:"), this), 1, 0);
	edit_layout->addWidget(m_replace_string, 1, 1);

	// Create options
	m_whole_words = new QCheckBox(tr("Match whole words only"), this);
	m_match_case = new QCheckBox(tr("Match case"), this);
	m_search_backwards = new QRadioButton(tr("Search up"), this);
	QRadioButton* search_forwards = new QRadioButton(tr("Search down"), this);
	search_forwards->setChecked(true);

	QGridLayout* options_layout = new QGridLayout;
	options_layout->setMargin(12);
	options_layout->setColumnStretch(1, 1);
	options_layout->addWidget(m_whole_words, 0, 0);
	options_layout->addWidget(m_match_case, 1, 0);
	options_layout->addWidget(m_search_backwards, 0, 2);
	options_layout->addWidget(search_forwards, 1, 2);

	// Create buttons
	QPushButton* find_button = new QPushButton(tr("Find"), this);
	connect(find_button, SIGNAL(clicked()), this, SLOT(find()));

	QPushButton* replace_button = new QPushButton(tr("Replace"), this);
	connect(replace_button, SIGNAL(clicked()), this, SLOT(replace()));

	QPushButton* replace_all_button = new QPushButton(tr("Replace All"), this);
	connect(replace_all_button, SIGNAL(clicked()), this, SLOT(replaceAll()));

	QPushButton* close_button = new QPushButton(tr("Close"), this);
	connect(close_button, SIGNAL(clicked()), this, SLOT(hide()));

	QHBoxLayout* buttons_layout = new QHBoxLayout;
	buttons_layout->addStretch();
	buttons_layout->addWidget(close_button);
	buttons_layout->addWidget(replace_all_button);
	buttons_layout->addWidget(replace_button);
	buttons_layout->addWidget(find_button);

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addLayout(edit_layout);
	layout->addLayout(options_layout);
	layout->addStretch();
	layout->addLayout(buttons_layout);

	// Load settings
	QSettings settings;
	m_match_case->setChecked(settings.value("FindDialog/CaseSensitive", false).toBool());
	m_whole_words->setChecked(settings.value("FindDialog/WholeWords", false).toBool());
	m_search_backwards->setChecked(settings.value("FindDialog/SearchBackwards", false).toBool());
}

/*****************************************************************************/

void FindDialog::hideEvent(QHideEvent* event) {
	QSettings settings;
	settings.setValue("FindDialog/CaseSensitive", m_match_case->isChecked());
	settings.setValue("FindDialog/WholeWords", m_whole_words->isChecked());
	settings.setValue("FindDialog/SearchBackwards", m_search_backwards->isChecked());
	QDialog::hideEvent(event);
}

/*****************************************************************************/

void FindDialog::showEvent(QShowEvent* event) {
	QString text = m_document->textCursor().selectedText().trimmed();
	text.remove(0, text.lastIndexOf(QChar(0x2029)) + 1);
	m_find_string->setText(text);
	m_replace_string->clear();
	QDialog::showEvent(event);
}

/*****************************************************************************/

void FindDialog::find() {
	QTextDocument::FindFlags flags;
	if (m_match_case->isChecked()) {
		flags |= QTextDocument::FindCaseSensitively;
	}
	if (m_whole_words->isChecked()) {
		flags |= QTextDocument::FindWholeWords;
	}
	if (m_search_backwards->isChecked()) {
		flags |= QTextDocument::FindBackward;
	}

	QTextCursor cursor = m_document->document()->find(m_find_string->text(), m_document->textCursor(), flags);
	if (cursor.isNull()) {
		cursor = m_document->textCursor();
		if (!m_search_backwards->isChecked()) {
			cursor.movePosition(QTextCursor::Start);
		} else {
			cursor.movePosition(QTextCursor::End);
		}
		cursor = m_document->document()->find(m_find_string->text(), cursor, flags);
	}

	if (!cursor.isNull()) {
		m_document->setTextCursor(cursor);
	} else {
		QMessageBox::information(this, tr("Sorry"), tr("Phrase not found."));
	}
}

/*****************************************************************************/

void FindDialog::replace() {
	QTextCursor cursor = m_document->textCursor();
	if (cursor.selectedText() == m_find_string->text()) {
		cursor.insertText(m_replace_string->text());
		m_document->setTextCursor(cursor);
	} else {
		return find();
	}
}

/*****************************************************************************/

void FindDialog::replaceAll() {
	QTextDocument::FindFlags flags;
	if (m_match_case->isChecked()) {
		flags |= QTextDocument::FindCaseSensitively;
	}
	if (m_whole_words->isChecked()) {
		flags |= QTextDocument::FindWholeWords;
	}

	// Count instances
	int found = 0;
	QTextCursor cursor = m_document->textCursor();
	cursor.movePosition(QTextCursor::Start);
	forever {
		cursor = m_document->document()->find(m_find_string->text(), cursor, flags);
		if (!cursor.isNull()) {
			found++;
		} else {
			break;
		}
	}
	if (found) {
		if (QMessageBox::question(this, tr("Question"), tr("Replace %1 instances?").arg(found), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
			return;
		}
	} else {
		QMessageBox::information(this, tr("Sorry"), tr("Phrase not found."));
		return;
	}

	// Replace instances
	QTextCursor start_cursor = m_document->textCursor();
	forever {
		cursor = m_document->document()->find(m_find_string->text(), cursor, flags);
		if (!cursor.isNull()) {
			cursor.insertText(m_replace_string->text());
			m_document->setTextCursor(cursor);
		} else {
			break;
		}
	}
	m_document->setTextCursor(start_cursor);
}

/*****************************************************************************/
