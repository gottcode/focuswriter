/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef DELTAS_H
#define DELTAS_H

class Document;

class Deltas
{
public:
	Deltas(Document* document);

	int characterCount() const;
	int characterAndSpaceCount() const;
	int pageCount() const;
	int paragraphCount() const;
	int wordCount() const;

	void refresh();

private:
	Document* m_document;
	int m_character_count;
	int m_character_and_space_count;
	int m_page_count;
	int m_paragraph_count;
	int m_word_count;
};

#endif
