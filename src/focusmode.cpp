/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "focusmode.h"

#include "block_stats.h"

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QTextEdit>
#include <stdio.h>

//-----------------------------------------------------------------------------

FocusMode::FocusMode(QTextEdit* text)
	: QSyntaxHighlighter(text),
	m_text(text),
	m_enabled(true),
	m_blurred_text("#666666"),
	m_changed(false)
{
	connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
}

//-----------------------------------------------------------------------------

void FocusMode::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		if(enabled) printf("FocusMode on\n");
		if(!enabled) printf("FocusMode off\n");
		rehighlight();
	}
}

//-----------------------------------------------------------------------------

void FocusMode::setBlurredTextColor(const QColor& color)
{
	if (m_blurred_text != color) {
		m_blurred_text = color;
		if (m_enabled) {
			rehighlight();
		}
	}
}

//-----------------------------------------------------------------------------

void FocusMode::cursorPositionChanged()
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

void FocusMode::highlightBlock(const QString& text)
{
	if (!m_enabled) {
		return;
	}

	QTextCharFormat format;
	//format.setForeground(m_blurred_text);
	format.setForeground(QColor::fromRgb(66, 66, 66, 128));

    int absolutePosition = m_text->textCursor().position();
    int inBlockPosition  = absolutePosition - m_text->textCursor().block().position();

	if(inBlockPosition == 0) {
	    setFormat(0, text.length(), format);
	    return;
	}

    // Text is current fragment(paragraph's) text only
    // Therefore where does out sentence begin?
    // see inBlockPosition above.
    int delta = -1;
    if(inBlockPosition > 0) {
        for(delta = inBlockPosition - 1; delta >= 0; delta--) {
            QChar ch = text.at(delta);
            printf("Char: [%d][%c]\n", delta, ch.toAscii());
            if(!ch.isLetterOrNumber() && !ch.isSpace()) {
                break;
            }
        }
        ++ delta;
        // This block
        setFormat(0, delta, format);
    }

    // OK, where does it end?
    int l = text.length();
    delta = l;

    if(inBlockPosition < l) {
        for(delta = inBlockPosition + 1; delta <= l; delta++) {
            QChar ch = text.at(delta);
            printf("Char: [%d][%c]\n", delta, ch.toAscii());
            if(!ch.isLetterOrNumber() && !ch.isSpace()) {
                break;
            }
        }
        ++ delta;
        // This block
        setFormat(delta, (l - delta), format);
    }
}

//-----------------------------------------------------------------------------

void FocusMode::reFocusVisually()
{
}
