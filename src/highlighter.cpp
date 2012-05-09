/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "highlighter.h"

#include "block_stats.h"
#include "dictionary.h"
#include "dictionary_manager.h"
#include "spell_checker.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QTextEdit>

//-----------------------------------------------------------------------------

Highlighter::Highlighter(QTextEdit* text, Dictionary** dictionary)
	: QSyntaxHighlighter(text),
	m_dictionary(dictionary),
	m_text(text),
	m_enabled(true),
	m_misspelled("#ff0000"),
	m_changed(false)
{
	connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));

	m_text->viewport()->installEventFilter(this);
	m_add_action = new QAction(tr("Add"), this);
	m_check_action = new QAction(tr("Check Spelling..."), this);
}

//-----------------------------------------------------------------------------

void Highlighter::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		rehighlight();
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
	if (watched != m_text->viewport() || event->type() != QEvent::ContextMenu || !m_enabled) {
		return QSyntaxHighlighter::eventFilter(watched, event);
	} else {
		// Check spelling of text block under mouse
		QContextMenuEvent* context_event = static_cast<QContextMenuEvent*>(event);
		m_start_cursor = m_text->cursorForPosition(context_event->pos());
		QTextBlock block = m_start_cursor.block();
		int cursor = m_start_cursor.position() - block.position();

		bool under_mouse = false;
		QStringRef word;
		QVector<QStringRef> words = static_cast<BlockStats*>(block.userData())->misspelled();
		for (int i = 0; i < words.count(); ++i) {
			word = words.at(i);
			int delta = cursor - word.position();
			if (delta >= 0 && delta <= word.length()) {
				under_mouse = true;
				break;
			}
		}

		if (!under_mouse) {
			return false;
		} else {
			// Select misspelled word
			m_cursor = m_start_cursor;
			m_cursor.setPosition(word.position() + block.position());
			m_cursor.setPosition(m_cursor.position() + word.length(), QTextCursor::KeepAnchor);
			m_word = m_cursor.selectedText();
			m_text->setTextCursor(m_cursor);

			// List suggestions in context menu
			QMenu* menu = new QMenu;
			QStringList guesses = (*m_dictionary)->suggestions(m_word);
			if (!guesses.isEmpty()) {
				foreach (const QString& guess, guesses) {
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
			connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(suggestion(QAction*)));
			menu->exec(context_event->globalPos());
			delete menu;

			return true;
		}
	}
}

//-----------------------------------------------------------------------------

void Highlighter::highlightBlock(const QString& text)
{
	if (!m_enabled) {
		return;
	}

	int cursor = m_text->textCursor().position() - currentBlock().position();
	BlockStats* stats = static_cast<BlockStats*>(currentBlockUserData());
	if (!stats) {
		stats = new BlockStats(text, m_dictionary);
		setCurrentBlockUserData(stats);
	}

	QTextCharFormat error;
	error.setUnderlineColor(m_misspelled);
	error.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

	QVector<QStringRef> words = stats->misspelled();
	for (int i = 0; i < words.count(); ++i) {
		const QStringRef& word = words.at(i);
		int delta = cursor - word.position();
		if (!m_changed || (delta < 0 || delta > word.length())) {
			setFormat(word.position(), word.length(), error);
		}
	}

	m_changed = true;
}

//-----------------------------------------------------------------------------

void Highlighter::cursorPositionChanged()
{
	QTextBlock current = m_text->textCursor().block();
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
		DictionaryManager::instance().add(m_word);
	} else if (action == m_check_action) {
		m_text->setTextCursor(m_start_cursor);
		SpellChecker::checkDocument(m_text);
	} else {
		m_cursor.insertText(action->text());
	}
}

//-----------------------------------------------------------------------------
