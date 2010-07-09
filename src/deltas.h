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
