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

#include "stats.h"

#include "block_stats.h"

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

Stats::Stats() :
	m_valid(false),
	m_character_count(0),
	m_letter_count(0),
	m_page_count(0),
	m_paragraph_count(0),
	m_space_count(0),
	m_word_count(0)
{
}

//-----------------------------------------------------------------------------

Stats::Stats(const Stats& stats) :
	m_valid(stats.m_valid),
	m_character_count(stats.m_character_count),
	m_letter_count(stats.m_letter_count),
	m_page_count(stats.m_page_count),
	m_paragraph_count(stats.m_paragraph_count),
	m_space_count(stats.m_space_count),
	m_word_count(stats.m_word_count)
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
