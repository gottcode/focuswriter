/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#include "deltas.h"

#include "document.h"

//-----------------------------------------------------------------------------

Deltas::Deltas(Document* document)
	: m_document(document)
{
	refresh();
}

//-----------------------------------------------------------------------------

int Deltas::characterCount() const
{
	return m_document->characterCount() - m_character_count;
}

//-----------------------------------------------------------------------------

int Deltas::characterAndSpaceCount() const
{
	return m_document->characterAndSpaceCount() - m_character_and_space_count;
}

//-----------------------------------------------------------------------------

int Deltas::pageCount() const
{
	return m_document->pageCount() - m_page_count;
}

//-----------------------------------------------------------------------------

int Deltas::paragraphCount() const
{
	return m_document->paragraphCount() - m_paragraph_count;
}

//-----------------------------------------------------------------------------

int Deltas::wordCount() const
{
	return m_document->wordCount() - m_word_count;
}

//-----------------------------------------------------------------------------

void Deltas::refresh()
{
	m_character_count = m_document->characterCount();
	m_character_and_space_count = m_document->characterAndSpaceCount();
	m_page_count = m_document->pageCount();
	m_paragraph_count = m_document->paragraphCount();
	m_word_count = m_document->wordCount();
}

//-----------------------------------------------------------------------------
