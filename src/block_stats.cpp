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

BlockStats::BlockStats(SceneModel* scene_model) :
	m_characters(0),
	m_spaces(0),
	m_words(0),
	m_scene(false),
	m_scene_model(scene_model),
	m_checked(true)
{
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

QList<QStringRef> BlockStats::checkSpelling(const QString& text, const Dictionary& dictionary)
{
	if (!m_checked) {
		m_misspelled.clear();
		if (!text.isEmpty()) {
			m_misspelled = dictionary.check(text);
		}
		m_checked = true;
	}
	return m_misspelled;
}

//-----------------------------------------------------------------------------

void BlockStats::update(const QString& text)
{
	m_checked = false;
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
}

//-----------------------------------------------------------------------------
