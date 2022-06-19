/*
	SPDX-FileCopyrightText: 2009-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_STATS_H
#define FOCUSWRITER_STATS_H

class BlockStats;

class Stats
{
public:
	explicit Stats();
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

#endif // FOCUSWRITER_STATS_H
