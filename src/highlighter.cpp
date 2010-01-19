/***********************************************************************
 *
 * Copyright (C) 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary.h"
#include "spell_checker.h"

#include <QAction>
#include <QMenu>
#include <QPlainTextEdit>

/*****************************************************************************/

Highlighter::Highlighter(QPlainTextEdit* text)
: QSyntaxHighlighter(text->document()),
  m_text(text),
  m_enabled(true),
  m_misspelled("#ff0000") {
	m_dictionary = new Dictionary(this);
	connect(m_dictionary, SIGNAL(changed()), this, SLOT(rehighlight()));

	m_text->viewport()->installEventFilter(this);
	m_add_action = new QAction(tr("Add"), this);
	m_check_action = new QAction(tr("Check Spelling..."), this);
}

/*****************************************************************************/

bool Highlighter::enabled() {
	return m_enabled;
}

/*****************************************************************************/

void Highlighter::setEnabled(bool enabled) {
	m_enabled = enabled;
	rehighlight();
}

/*****************************************************************************/

void Highlighter::setMisspelledColor(const QColor& color) {
	m_misspelled = color;
	rehighlight();
}

/*****************************************************************************/

bool Highlighter::eventFilter(QObject* watched, QEvent* event) {
	if (watched != m_text->viewport() || event->type() != QEvent::ContextMenu || !m_enabled) {
		return QSyntaxHighlighter::eventFilter(watched, event);
	} else {
		// Check spelling of text block under mouse
		QContextMenuEvent* context_event = static_cast<QContextMenuEvent*>(event);
		m_start_cursor = m_text->cursorForPosition(context_event->pos());
		QTextBlock block = m_start_cursor.block();
		QString text = block.text();
		int cursor = m_start_cursor.position() - block.position();

		bool under_mouse = false;
		QStringRef word;
		while ((word = m_dictionary->check(text, word.position() + word.length())).isNull() == false) {
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
			QStringList guesses = m_dictionary->suggestions(m_word);
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

/*****************************************************************************/

void Highlighter::highlightBlock(const QString& text) {
	if (!m_enabled) {
		return;
	}

	QTextCharFormat error;
	error.setUnderlineColor(m_misspelled);
	error.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	QStringRef word;
	while ((word = m_dictionary->check(text, word.position() + word.length())).isNull() == false) {
		setFormat(word.position(), word.length(), error);
	}
}

/*****************************************************************************/

void Highlighter::suggestion(QAction* action) {
	if (action == m_add_action) {
		m_text->setTextCursor(m_start_cursor);
		m_dictionary->add(m_word);
	} else if (action == m_check_action) {
		m_text->setTextCursor(m_start_cursor);
		SpellChecker::checkDocument(m_text);
	} else {
		m_cursor.insertText(action->text());
	}
}

/*****************************************************************************/
