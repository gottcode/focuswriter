/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#ifndef BLOCK_STATS_H
#define BLOCK_STATS_H

class DictionaryRef;
class SceneModel;

#include <QTextBlockUserData>

class BlockStats : public QTextBlockUserData
{
public:
	BlockStats(SceneModel* scene_model);
	~BlockStats();

	bool isEmpty() const;
	bool isScene() const;
	int characterCount() const;
	int letterCount() const;
	int spaceCount() const;
	int wordCount() const;
	QVector<QStringRef> misspelled() const;

	enum SpellCheckStatus
	{
		Unchecked,
		Checked,
		CheckSpelling
	};
	SpellCheckStatus spellingStatus() const;

	void checkSpelling(const QString& text, const DictionaryRef& dictionary);
	void recheckSpelling();
	void setScene(bool scene);
	void update(const QString& text);

private:
	int m_characters;
	int m_letters;
	int m_spaces;
	int m_words;
	bool m_scene;
	SceneModel* m_scene_model;
	QVector<QStringRef> m_misspelled;
	SpellCheckStatus m_checked;
};

inline bool BlockStats::isEmpty() const
{
	return m_words == 0;
}

inline bool BlockStats::isScene() const
{
	return m_scene;
}

inline int BlockStats::characterCount() const
{
	return m_characters;
}

inline int BlockStats::letterCount() const
{
	return m_letters;
}

inline int BlockStats::spaceCount() const
{
	return m_spaces;
}

inline int BlockStats::wordCount() const
{
	return m_words;
}

inline QVector<QStringRef> BlockStats::misspelled() const
{
	return m_misspelled;
}

inline void BlockStats::setScene(bool scene)
{
	m_scene = scene;
}

inline BlockStats::SpellCheckStatus BlockStats::spellingStatus() const
{
	return m_checked;
}

inline void BlockStats::recheckSpelling()
{
	m_checked = CheckSpelling;
}

#endif
