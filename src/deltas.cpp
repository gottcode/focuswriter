/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "deltas.h"

#include "document.h"

//-----------------------------------------------------------------------------

Deltas::Deltas(const Document* document)
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
