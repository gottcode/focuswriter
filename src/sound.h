/*
	SPDX-FileCopyrightText: 2010-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef SOUND_H
#define SOUND_H

#include <QObject>
#include <QVector>
class QSoundEffect;

class Sound : public QObject
{
public:
	Sound(int name, const QString& filename, QObject* parent = 0);
	~Sound();

	bool isValid() const;
	void play();

	static void play(int name);
	static void setEnabled(bool enabled);
	static void setPath(const QString& path);

private:
	int m_name;
	QVector<QSoundEffect*> m_sounds;
};

#endif
