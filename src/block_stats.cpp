/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "block_stats.h"

#include "dictionary.h"
#include "scene_model.h"

//-----------------------------------------------------------------------------

static QString f_scene_divider = QLatin1String("##");

//-----------------------------------------------------------------------------

BlockStats::BlockStats(int block_number, const QString& text, Dictionary* dictionary, SceneModel* scene_model) :
	m_characters(0),
	m_spaces(0),
	m_words(0),
	m_scene(false),
	m_scene_model(scene_model)
{
	update(block_number, text, dictionary);
}

//-----------------------------------------------------------------------------

BlockStats::~BlockStats()
{
	if (m_scene) {
		Q_ASSERT(m_scene_model != 0);
		m_scene_model->removeScene(this);
	}
}

//-----------------------------------------------------------------------------

void BlockStats::checkSpelling(const QString& text, Dictionary* dictionary)
{
	m_misspelled.clear();
	if (text.isEmpty() || !dictionary) {
		return;
	}
	m_misspelled = dictionary->check(text);
}

//-----------------------------------------------------------------------------

void BlockStats::update(int block_number, const QString& text, Dictionary* dictionary)
{
	// Determine if this is a scene
	if (m_scene_model) {
		bool was_scene = m_scene;
		m_scene = text.startsWith(f_scene_divider);
		if (m_scene) {
			QString scene_text = text.mid(f_scene_divider.length()).trimmed();
			if (was_scene) {
				m_scene_model->updateScene(this, scene_text);
			} else {
				m_scene_model->addScene(this, block_number, scene_text);
			}
		} else if (was_scene) {
			m_scene_model->removeScene(this);
		}
	}

	// Calculate stats
	m_characters = text.length();
	m_spaces = 0;
	m_words = 0;
	bool word = false;
	QString::const_iterator end = text.constEnd();
	for (QString::const_iterator i = text.constBegin(); i != end; ++i) {
		if (i->isLetterOrNumber() || i->category() == QChar::Punctuation_Dash) {
			if (word == false) {
				word = true;
				m_words++;
			}
		} else if (i->isSpace()) {
			word = false;
			m_spaces++;
		} else if (*i != 0x2019 && *i != 0x0027) {
			word = false;
		}
	}

	// Update stored list of misspelled words
	checkSpelling(text, dictionary);
}

//-----------------------------------------------------------------------------

void BlockStats::setSceneDivider(const QString& divider)
{
	f_scene_divider = divider;
	f_scene_divider.replace(QLatin1String("\\t"), QLatin1String("\t"));
}

//-----------------------------------------------------------------------------
