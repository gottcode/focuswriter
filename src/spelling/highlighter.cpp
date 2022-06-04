/*
	SPDX-FileCopyrightText: 2009-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "highlighter.h"

#include "block_stats.h"
#include "dictionary_ref.h"
#include "spell_checker.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QTextEdit>
#include <QTimer>

//-----------------------------------------------------------------------------

Highlighter::Highlighter(QTextEdit* text, DictionaryRef& dictionary)
	: QSyntaxHighlighter(text)
	, m_dictionary(dictionary)
	, m_text(text)
	, m_enabled(true)
	, m_misspelled(0xff, 0, 0)
	, m_changed(false)
{
	connect(m_text, &QTextEdit::cursorPositionChanged, this, &Highlighter::cursorPositionChanged);

	m_spell_timer = new QTimer(this);
	m_spell_timer->setInterval(10);
	m_spell_timer->setSingleShot(true);
	connect(m_spell_timer, &QTimer::timeout, this, &Highlighter::updateSpelling);

	m_text->installEventFilter(this);
	m_text->viewport()->installEventFilter(this);
	m_add_action = new QAction(tr("Add"), this);
	m_check_action = new QAction(tr("Check Spelling..."), this);
}

//-----------------------------------------------------------------------------

void Highlighter::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		if (m_enabled) {
			updateSpelling();
		} else {
			rehighlight();
		}
	}
}

//-----------------------------------------------------------------------------

void Highlighter::setMisspelledColor(const QColor& color)
{
	if (m_misspelled != color) {
		m_misspelled = color;
		if (m_enabled) {
			rehighlight();
		}
	}
}

//-----------------------------------------------------------------------------

bool Highlighter::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() != QEvent::ContextMenu || !m_enabled || m_text->isReadOnly()) {
		return QSyntaxHighlighter::eventFilter(watched, event);
	} else {
		// Check spelling of text block under mouse
		const QContextMenuEvent* context_event = static_cast<const QContextMenuEvent*>(event);
		m_start_cursor = m_text->cursorForPosition(context_event->pos());
		const QTextBlock block = m_start_cursor.block();
		const int cursor = m_start_cursor.position() - block.position();

		const QList<WordRef> words = static_cast<BlockStats*>(block.userData())->misspelled();
		for (const WordRef& word : words) {
			const int delta = cursor - word.position();
			if (delta < 0 || delta > word.length()) {
				continue;
			}

			// Select misspelled word
			m_cursor = m_start_cursor;
			m_cursor.setPosition(word.position() + block.position());
			m_cursor.setPosition(m_cursor.position() + word.length(), QTextCursor::KeepAnchor);
			m_word = m_cursor.selectedText();
			m_text->blockSignals(true);
			m_text->setTextCursor(m_cursor);
			m_text->blockSignals(false);

			// List suggestions in context menu
			QMenu* menu = new QMenu;
			const QStringList guesses = m_dictionary.suggestions(m_word);
			if (!guesses.isEmpty()) {
				for (const QString& guess : guesses) {
					menu->addAction(guess);
				}
			} else {
				QAction* none_action = menu->addAction(tr("(No suggestions found)"));
				none_action->setEnabled(false);
			}
			menu->addSeparator();
			menu->addAction(m_add_action);
			menu->addSeparator();
			menu->addAction(m_check_action);

			// Show menu
			connect(menu, &QMenu::triggered, this, &Highlighter::suggestion);
			menu->exec(context_event->globalPos());
			delete menu;

			return true;
		}

		return false;
	}
}

//-----------------------------------------------------------------------------

void Highlighter::highlightBlock(const QString& text)
{
	QTextCharFormat style;
	const int heading = currentBlock().blockFormat().property(QTextFormat::UserProperty).toInt();
	if (heading) {
		style.setProperty(QTextFormat::FontSizeAdjustment, 4 - heading);
		style.setFontWeight(QFont::Bold);
		setFormat(0, text.length(), style);
	}

	BlockStats* stats = static_cast<BlockStats*>(currentBlockUserData());
	if (!m_enabled || m_text->isReadOnly() || !stats || (stats->spellingStatus() == BlockStats::Unchecked)) {
		return;
	}
	if (stats->spellingStatus() == BlockStats::CheckSpelling) {
		stats->checkSpelling(text, m_dictionary);
	}

	style.setUnderlineColor(m_misspelled);
	style.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

	const int cursor = m_text->textCursor().position() - currentBlock().position();
	const QList<WordRef> words = stats->misspelled();
	for (const WordRef& word : words) {
		const int delta = cursor - word.position();
		if (!m_changed || (delta < 0 || delta > word.length())) {
			setFormat(word.position(), word.length(), style);
		}
	}

	m_changed = true;
}

//-----------------------------------------------------------------------------

void Highlighter::updateSpelling()
{
	if (!m_enabled || m_text->isReadOnly()) {
		return;
	}

	const QTextBlock block = m_text->textCursor().block();
	bool found = false;

	// Check first unchecked block at or after cursor
	for (QTextBlock i = block; i.isValid(); i = i.next()) {
		BlockStats* stats = static_cast<BlockStats*>(i.userData());
		if (stats && (stats->spellingStatus() != BlockStats::Checked)) {
			stats->checkSpelling(i.text(), m_dictionary);
			rehighlightBlock(i);
			found = true;
			break;
		}
	}

	// Check first unchecked block before cursor
	for (QTextBlock i = block; i.isValid(); i = i.previous()) {
		BlockStats* stats = static_cast<BlockStats*>(i.userData());
		if (stats && (stats->spellingStatus() != BlockStats::Checked)) {
			stats->checkSpelling(i.text(), m_dictionary);
			rehighlightBlock(i);
			found = true;
			break;
		}
	}

	// Repeat until all blocks have been checked
	if (found) {
		m_spell_timer->start();
	}
}

//-----------------------------------------------------------------------------

void Highlighter::cursorPositionChanged()
{
	const QTextBlock current = m_text->textCursor().block();
	if (m_current != current) {
		if (m_current.isValid() && m_text->document()->blockCount() > m_current.blockNumber()) {
			rehighlightBlock(m_current);
		}
		m_current = current;
	}
	rehighlightBlock(m_current);
	m_changed = false;
}

//-----------------------------------------------------------------------------

void Highlighter::suggestion(QAction* action)
{
	if (action == m_add_action) {
		m_text->setTextCursor(m_start_cursor);
		m_dictionary.addToPersonal(m_word);
	} else if (action == m_check_action) {
		m_text->setTextCursor(m_start_cursor);
		SpellChecker::checkDocument(m_text, m_dictionary);
	} else {
		m_cursor.insertText(action->text());
	}
}

//-----------------------------------------------------------------------------
