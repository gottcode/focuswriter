/*
	SPDX-FileCopyrightText: 2010-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SOUND_H
#define FOCUSWRITER_SOUND_H

#include <QList>
#include <QObject>
class QSoundEffect;

class Sound : public QObject
{
public:
	Sound(int name, const QString& filename, QObject* parent = nullptr);
	~Sound();

	bool isValid() const;
	void play();

	static void play(int name);
	static void setEnabled(bool enabled);
	static void setPath(const QString& path);

private:
	int m_name;
	QList<QSoundEffect*> m_sounds;
};

#endif // FOCUSWRITER_SOUND_H
