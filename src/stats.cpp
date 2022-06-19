/*
	SPDX-FileCopyrightText: 2009-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "stats.h"

#include "block_stats.h"

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

Stats::Stats()
	: m_valid(false)
	, m_character_count(0)
	, m_letter_count(0)
	, m_page_count(0)
	, m_paragraph_count(0)
	, m_space_count(0)
	, m_word_count(0)
{
}

//-----------------------------------------------------------------------------

Stats::Stats(const Stats& stats)
	: m_valid(stats.m_valid)
	, m_character_count(stats.m_character_count)
	, m_letter_count(stats.m_letter_count)
	, m_page_count(stats.m_page_count)
	, m_paragraph_count(stats.m_paragraph_count)
	, m_space_count(stats.m_space_count)
	, m_word_count(stats.m_word_count)
{
}

//-----------------------------------------------------------------------------

Stats& Stats::operator=(const Stats& stats)
{
	m_valid = stats.m_valid;
	m_character_count = stats.m_character_count;
	m_letter_count = stats.m_letter_count;
	m_page_count = stats.m_page_count;
	m_paragraph_count = stats.m_paragraph_count;
	m_space_count = stats.m_space_count;
	m_word_count = stats.m_word_count;
	return *this;
}

//-----------------------------------------------------------------------------

void Stats::append(const BlockStats* block)
{
	m_valid = true;
	m_character_count += block->characterCount();
	m_letter_count += block->letterCount();
	m_paragraph_count += !block->isEmpty();
	m_space_count += block->spaceCount();
	m_word_count += block->wordCount();
}

//-----------------------------------------------------------------------------

void Stats::calculatePageCount(int type, float page_amount)
{
	float amount = 0;
	switch (type) {
	case 1:
		amount = m_paragraph_count;
		break;
	case 2:
		amount = m_word_count;
		break;
	default:
		amount = m_character_count;
		break;
	}
	m_page_count = std::max(1.0f, std::ceil(amount / page_amount));
}

//-----------------------------------------------------------------------------

void Stats::calculateWordCount(int type)
{
	switch (type) {
	case 1:
		m_word_count = std::ceil(m_character_count / 6.0f);
		break;
	case 2:
		m_word_count = m_letter_count;
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------

void Stats::clear()
{
	m_valid = false;
	m_character_count = 0;
	m_letter_count = 0;
	m_page_count = 0;
	m_paragraph_count = 0;
	m_space_count = 0;
	m_word_count = 0;
}

//-----------------------------------------------------------------------------
