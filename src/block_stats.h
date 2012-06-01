/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2012 Graeme Gott <graeme@gottcode.org>
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

#include <QTextBlockUserData>

class Dictionary;

class BlockStats : public QTextBlockUserData
{
public:
	BlockStats(const QString& text, Dictionary* dictionary);

	bool isEmpty() const;
	bool isScene() const;
	int characterCount() const;
	int spaceCount() const;
	int wordCount() const;
	QList<QStringRef> misspelled() const;

	void checkSpelling(const QString& text, Dictionary* dictionary);
	void update(const QString& text, Dictionary* dictionary);

	static void setSceneDivider(const QString& divider);

private:
	int m_characters;
	int m_spaces;
	int m_words;
	bool m_scene;
	QList<QStringRef> m_misspelled;
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

inline int BlockStats::spaceCount() const
{
	return m_spaces;
}

inline int BlockStats::wordCount() const
{
	return m_words;
}

inline QList<QStringRef> BlockStats::misspelled() const
{
	return m_misspelled;
}

#endif
