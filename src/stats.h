/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2014 Graeme Gott <graeme@gottcode.org>
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

class BlockStats;

class Stats
{
public:
	Stats();
	Stats(const Stats& stats);
	Stats& operator=(const Stats& stats);

	bool isValid() const;
	int characterCount() const;
	int characterAndSpaceCount() const;
	int pageCount() const;
	int paragraphCount() const;
	int wordCount() const;

	void append(const BlockStats* block);
	void calculatePageCount(int type, float page_amount);
	void calculateWordCount(int type);
	void clear();

private:
	bool m_valid;
	int m_character_count;
	int m_letter_count;
	int m_page_count;
	int m_paragraph_count;
	int m_space_count;
	int m_word_count;
};

inline bool Stats::isValid() const
{
	return m_valid;
}

inline int Stats::characterCount() const
{
	return m_character_count - m_space_count;
}

inline int Stats::characterAndSpaceCount() const
{
	return m_character_count;
}

inline int Stats::pageCount() const
{
	return m_page_count;
}

inline int Stats::paragraphCount() const
{
	return m_paragraph_count;
}

inline int Stats::wordCount() const
{
	return m_word_count;
}
