/*
	SPDX-FileCopyrightText: 2009 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_BLOCK_STATS_H
#define FOCUSWRITER_BLOCK_STATS_H

#include "word_ref.h"
class DictionaryRef;
class SceneModel;

#include <QTextBlockUserData>

class BlockStats : public QTextBlockUserData
{
public:
	explicit BlockStats(SceneModel* scene_model);
	~BlockStats();

	bool isEmpty() const;
	bool isScene() const;
	qint64 characterCount() const;
	qint64 letterCount() const;
	qint64 spaceCount() const;
	qint64 wordCount() const;
	QList<WordRef> misspelled() const;

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
	qint64 m_characters;
	qint64 m_letters;
	qint64 m_spaces;
	qint64 m_words;
	bool m_scene;
	SceneModel* m_scene_model;
	QList<WordRef> m_misspelled;
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

inline qint64 BlockStats::characterCount() const
{
	return m_characters;
}

inline qint64 BlockStats::letterCount() const
{
	return m_letters;
}

inline qint64 BlockStats::spaceCount() const
{
	return m_spaces;
}

inline qint64 BlockStats::wordCount() const
{
	return m_words;
}

inline QList<WordRef> BlockStats::misspelled() const
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

#endif // FOCUSWRITER_BLOCK_STATS_H
