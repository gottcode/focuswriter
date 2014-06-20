/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "../sound.h"

#include <QHash>
#include <QList>
#include <QSound>

//-----------------------------------------------------------------------------

namespace
{
	// Shared data
	QString f_path;
	bool f_enabled = false;

	QList<QList<QSound*> > f_sounds;
	int f_total_sounds = 0;
	QHash<QString, int> f_ids;
	QHash<int, Sound*> f_sound_objects;
}

//-----------------------------------------------------------------------------

Sound::Sound(int name, const QString& filename, QObject* parent) :
	QObject(parent),
	m_id(-1),
	m_name(name)
{
	++f_total_sounds;

	if (f_ids.contains(filename)) {
		m_id = f_ids.value(filename);
	} else {
		m_id = f_sounds.count();
		QSound* sound = new QSound(f_path + "/" + filename);
		f_sounds.append(QList<QSound*>() << sound);
		f_ids[filename] = m_id;
	}

	f_sound_objects[m_name] = this;
}

//-----------------------------------------------------------------------------

Sound::~Sound()
{
	f_sound_objects[m_name] = 0;
	--f_total_sounds;
	if (f_total_sounds == 0) {
		int count = f_sounds.count();
		for (int i = 0; i < count; ++i) {
			qDeleteAll(f_sounds[i]);
		}
		f_sounds.clear();
		f_ids.clear();
	}
}

//-----------------------------------------------------------------------------

void Sound::play(int name)
{
	if (!f_enabled) {
		return;
	}

	Sound* sound = f_sound_objects.value(name);
	if (!sound || !sound->isValid()) {
		return;
	}

	QSound* qsound = 0;
	QList<QSound*>& sounds = f_sounds[sound->m_id];
	int count = sounds.count();
	for (int i = 0; i < count; ++i) {
		if (sounds.at(i)->isFinished()) {
			qsound = sounds.at(i);
			break;
		}
	}
	if (qsound == 0) {
		qsound = new QSound(sounds.first()->fileName());
		sounds.append(qsound);
	}
	qsound->play();
}

//-----------------------------------------------------------------------------

void Sound::setEnabled(bool enabled)
{
	f_enabled = enabled;
}

//-----------------------------------------------------------------------------

void Sound::setPath(const QString& path)
{
	f_path = path;
}

//-----------------------------------------------------------------------------
