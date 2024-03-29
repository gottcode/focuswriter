/*
	SPDX-FileCopyrightText: 2009-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "spell_checker.h"

#include "dictionary_ref.h"
#include "document.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextBlock>
#include <QTextEdit>

//-----------------------------------------------------------------------------

void SpellChecker::checkDocument(QTextEdit* document, DictionaryRef& dictionary)
{
	SpellChecker* checker = new SpellChecker(document, dictionary);
	checker->m_start_cursor = document->textCursor();
	checker->m_cursor = checker->m_start_cursor;
	checker->m_cursor.movePosition(QTextCursor::StartOfBlock);
	checker->m_loop_available = checker->m_start_cursor.block().previous().isValid();
	checker->show();
	checker->check();
}

//-----------------------------------------------------------------------------

void SpellChecker::reject()
{
	m_document->setTextCursor(m_start_cursor);
	Document* document = qobject_cast<Document*>(m_document->parentWidget());
	if (document) {
		document->centerCursor(true);
	}
	QDialog::reject();
}

//-----------------------------------------------------------------------------

void SpellChecker::suggestionChanged(const QListWidgetItem* suggestion)
{
	if (suggestion) {
		m_suggestion->setText(suggestion->text());
	}
}

//-----------------------------------------------------------------------------

void SpellChecker::add()
{
	m_dictionary.addToPersonal(m_word);
	ignore();
}

//-----------------------------------------------------------------------------

void SpellChecker::ignore()
{
	check();
}

//-----------------------------------------------------------------------------

void SpellChecker::ignoreAll()
{
	m_ignored.append(m_word);
	ignore();
}

//-----------------------------------------------------------------------------

void SpellChecker::change()
{
	m_cursor.insertText(m_suggestion->text());
	check();
}

//-----------------------------------------------------------------------------

void SpellChecker::changeAll()
{
	const QString replacement = m_suggestion->text();

	QTextCursor cursor = m_cursor;
	cursor.movePosition(QTextCursor::Start);
	Q_FOREVER {
		cursor = m_document->document()->find(m_word, cursor, QTextDocument::FindCaseSensitively | QTextDocument::FindWholeWords);
		if (!cursor.isNull()) {
			cursor.insertText(replacement);
		} else {
			break;
		}
	}

	check();
}

//-----------------------------------------------------------------------------

SpellChecker::SpellChecker(QTextEdit* document, DictionaryRef& dictionary)
	: QDialog(document->parentWidget(), Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
	, m_dictionary(dictionary)
	, m_document(document)
	, m_checked_blocks(1)
	, m_total_blocks(document->document()->blockCount())
	, m_loop_available(true)
{
	setWindowTitle(tr("Check Spelling"));
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose);

	// Create widgets
	m_context = new QTextEdit(this);
	m_context->setReadOnly(true);
	m_context->setTabStopDistance(50);
	QPushButton* add_button = new QPushButton(tr("&Add"), this);
	add_button->setAutoDefault(false);
	connect(add_button, &QPushButton::clicked, this, &SpellChecker::add);
	QPushButton* ignore_button = new QPushButton(tr("&Ignore"), this);
	ignore_button->setAutoDefault(false);
	connect(ignore_button, &QPushButton::clicked, this, &SpellChecker::ignore);
	QPushButton* ignore_all_button = new QPushButton(tr("I&gnore All"), this);
	ignore_all_button->setAutoDefault(false);
	connect(ignore_all_button, &QPushButton::clicked, this, &SpellChecker::ignoreAll);

	m_suggestion = new QLineEdit(this);
	QPushButton* change_button = new QPushButton(tr("&Change"), this);
	change_button->setAutoDefault(false);
	connect(change_button, &QPushButton::clicked, this, &SpellChecker::change);
	QPushButton* change_all_button = new QPushButton(tr("C&hange All"), this);
	change_all_button->setAutoDefault(false);
	connect(change_all_button, &QPushButton::clicked, this, &SpellChecker::changeAll);
	m_suggestions = new QListWidget(this);
	connect(m_suggestions, &QListWidget::currentItemChanged, this, &SpellChecker::suggestionChanged);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &SpellChecker::reject);

	// Lay out dialog
	QGridLayout* layout = new QGridLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(6);
	layout->setColumnMinimumWidth(2, 6);

	layout->addWidget(new QLabel(tr("Not in dictionary:"), this), 0, 0, 1, 2);
	layout->addWidget(m_context, 1, 0, 3, 2);
	layout->addWidget(add_button, 1, 3);
	layout->addWidget(ignore_button, 2, 3);
	layout->addWidget(ignore_all_button, 3, 3);

	layout->setRowMinimumHeight(4, 12);

	layout->addWidget(new QLabel(tr("Change to:"), this), 5, 0);
	layout->addWidget(m_suggestion, 5, 1);
	layout->addWidget(m_suggestions, 6, 0, 1, 2);
	layout->addWidget(change_button, 5, 3);
	layout->addWidget(change_all_button, 6, 3, Qt::AlignTop);

	layout->setRowMinimumHeight(7, 12);
	layout->addWidget(buttons, 8, 3);
}

//-----------------------------------------------------------------------------

void SpellChecker::check()
{
	setDisabled(true);

	QProgressDialog wait_dialog(tr("Checking spelling..."), tr("Cancel"), 0, m_total_blocks, this);
	wait_dialog.setWindowTitle(tr("Please wait"));
	wait_dialog.setValue(0);
	wait_dialog.setWindowModality(Qt::WindowModal);
	bool canceled = false;

	Q_FOREVER {
		// Update wait dialog
		wait_dialog.setValue(m_checked_blocks);
		if (wait_dialog.wasCanceled()) {
			canceled = true;
			break;
		}

		// Check current line
		const QTextBlock block = m_cursor.block();
		const WordRef word = m_dictionary.check(block.text(), m_cursor.position() - block.position());
		if (word.isNull()) {
			if (block.next().isValid()) {
				m_cursor.movePosition(QTextCursor::NextBlock);
				++m_checked_blocks;
				if (m_checked_blocks < m_total_blocks) {
					continue;
				} else {
					break;
				}
			} else if (m_loop_available) {
				wait_dialog.reset();
				if (QMessageBox::question(this,
						QString(),
						tr("Continue checking at beginning of file?"),
						QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
					m_loop_available = false;
					m_cursor.movePosition(QTextCursor::Start);
					wait_dialog.setRange(0, m_total_blocks);
					continue;
				} else {
					canceled = true;
					break;
				}
			} else {
				break;
			}
		}

		// Select misspelled word
		m_cursor.setPosition(word.position() + block.position());
		m_cursor.setPosition(m_cursor.position() + word.length(), QTextCursor::KeepAnchor);
		m_word = m_cursor.selectedText();

		if (!m_ignored.contains(m_word)) {
			wait_dialog.close();
			setEnabled(true);

			// Show misspelled word in context
			QTextCursor cursor = m_cursor;
			cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::MoveAnchor, 10);
			const int end = m_cursor.position() - cursor.position();
			const int start = end - m_word.length();
			cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor, 21);
			QString context = cursor.selectedText();
			context.insert(end, "</span>");
			context.insert(start, "<span style=\"color: red;\">");
			context.replace("\n", "</p><p>");
			context.replace("\t", "<span style=\"white-space: pre;\">\t</span>");
			context = "<p>" + context + "</p>";
			m_context->setHtml(context);

			// Show suggestions
			m_suggestion->clear();
			m_suggestions->clear();
			const QStringList words = m_dictionary.suggestions(m_word);
			if (!words.isEmpty()) {
				for (const QString& word : words) {
					m_suggestions->addItem(word);
				}
				m_suggestions->setCurrentRow(0);
			}

			// Stop checking words
			m_document->setTextCursor(m_cursor);
			m_suggestion->setFocus();
			return;
		}
	}

	// Inform user of completed spell check
	wait_dialog.close();
	if (!canceled) {
		QMessageBox::information(this, QString(), tr("Spell check complete."));
	}
	reject();
}

//-----------------------------------------------------------------------------
