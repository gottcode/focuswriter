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
	m_level(0),
	m_blurred_text("#666666"),
	m_changed(false)
{
	connect(m_text, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
}

//-----------------------------------------------------------------------------

void FocusMode::setLevel(const int level)
{
	if (m_level != level) {
		m_level = level;
		carefulRehighlight();
	}
}

//-----------------------------------------------------------------------------

void FocusMode::setBlurredTextColor(const QColor& color)
{
	if (m_blurred_text != color) {
		m_blurred_text = color;
		if (m_level) {
			carefulRehighlight();
		}
	}
}

//-----------------------------------------------------------------------------

void FocusMode::carefulRehighlight() {
    QTextDocument *doc = m_text->document();
    QTextBlock block = doc->begin();

    while(block.isValid()) {
        m_current = block;
        rehighlightBlock(block);
        block = block.next();
    }

    m_current = m_text->textCursor().block();
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
	if (!m_level) {
		return;
	}

	QTextCharFormat format;
	// TODO FIXME
	//format.setForeground(m_blurred_text);
	format.setForeground(QColor::fromRgb(66, 66, 66, 128));

    int absolutePosition = m_text->textCursor().position();
    int inBlockPosition  = absolutePosition - m_text->textCursor().block().position();

	if(inBlockPosition == 0 || m_current != m_text->textCursor().block()) {
	    setFormat(0, text.length(), format);
	    return;
	}

    // Text is current fragment(paragraph's) text only
    // Therefore where does out sentence begin?
    // see inBlockPosition above.
    int delta = -1;
    if(inBlockPosition > 0) {
        int sentences = m_level;
        for(delta = inBlockPosition - 1; delta >= 0; delta--) {
            QChar ch = text.at(delta);
//                printf("<- Char: [%d][%c]\n", delta, ch.toAscii());
            if(isADelimiter(ch)) {
                if(sentences != 3 && --sentences < 1) {
                    break;
                }
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
        int sentences = m_level;
        for(delta = inBlockPosition; delta <= l; delta++) {
            QChar ch = text.at(delta);
//                printf("-> Char: [%d][%c]\n", delta, ch.toAscii());
            if(isADelimiter(ch)) {
                if(sentences != 3 && --sentences < 1) {
                    break;
                }
            }
        }
        ++ delta;
        // This block
        setFormat(delta, (l - delta), format);
    }
}

//-----------------------------------------------------------------------------

/*
 * Considered delimiters:
 * ! . ; ?
 * Clearly not considered delimiters:
 * , - / : _ { | }
 * Ignored due to ignorance (how fitting):
 * all other candidates.
 */
bool FocusMode::isADelimiter(const QChar ch)
{
    ushort u = ch.unicode();
    return (u == 0x0021 || u == 0x002E || u == 0x003B || u == 0x003F);
}
