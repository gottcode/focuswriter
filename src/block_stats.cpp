/*
	SPDX-FileCopyrightText: 2009-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "block_stats.h"

#include "dictionary_ref.h"
#include "scene_model.h"

//-----------------------------------------------------------------------------

BlockStats::BlockStats(SceneModel* scene_model)
	: m_characters(0)
	, m_letters(0)
	, m_spaces(0)
	, m_words(0)
	, m_scene(false)
	, m_scene_model(scene_model)
{
}

//-----------------------------------------------------------------------------

BlockStats::~BlockStats()
{
	if (m_scene) {
		Q_ASSERT(m_scene_model);
		m_scene_model->removeScene(this);
	}
}

//-----------------------------------------------------------------------------

void BlockStats::checkSpelling(const QString& text, const DictionaryRef& dictionary)
{
	m_misspelled.clear();
	if (!text.isEmpty()) {
		WordRef word;
		while ((word = dictionary.check(text, word.position() + word.length())).isNull() == false) {
			m_misspelled.append(word);
		}
	}
	m_checked = Checked;
}

//-----------------------------------------------------------------------------

void BlockStats::update(const QString& text)
{
	m_checked = Unchecked;
	m_characters = text.length();
	m_letters = 0;
	m_spaces = 0;
	m_words = 0;
	bool word = false;
	for (const QChar& c : text) {
		if (c.isLetterOrNumber()) {
			if (word == false) {
				word = true;
				m_words++;
			}
			m_letters += (c.category() != QChar::Punctuation_Dash);
		} else if (c.isSpace()) {
			word = false;
			m_spaces++;
		} else if (c != u'’' && c != '\'' && c != '-') {
			word = false;
		}
	}
}

//-----------------------------------------------------------------------------
