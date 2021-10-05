/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012, 2013, 2016 Graeme Gott <graeme@gottcode.org>
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

#include "sound.h"

#include <QApplication>
#include <QHash>
#include <QSoundEffect>

//-----------------------------------------------------------------------------

namespace
{
	// Shared data
	QString f_path;
	bool f_enabled = false;

	QHash<int, Sound*> f_sound_objects;
}

//-----------------------------------------------------------------------------

Sound::Sound(int name, const QString& filename, QObject* parent) :
	QObject(parent),
	m_name(name)
{
	QSoundEffect* sound = new QSoundEffect(this);
	sound->setSource(QUrl::fromLocalFile(f_path + "/" + filename));
	while (sound->status() == QSoundEffect::Loading) {
		QApplication::processEvents();
	}
	m_sounds.append(sound);

	f_sound_objects.remove(m_name);
	if (sound->status() != QSoundEffect::Error) {
		f_sound_objects.insert(m_name, this);
	}
}

//-----------------------------------------------------------------------------

Sound::~Sound()
{
	f_sound_objects.remove(m_name);
}

//-----------------------------------------------------------------------------

bool Sound::isValid() const
{
	return f_sound_objects.contains(m_name);
}

//-----------------------------------------------------------------------------

void Sound::play()
{
	QSoundEffect* sound = nullptr;
	for (int i = 0, end = m_sounds.size(); i < end; ++i) {
		if (!m_sounds.at(i)->isPlaying()) {
			sound = m_sounds.at(i);
			break;
		}
	}
	if (sound == nullptr) {
		sound = new QSoundEffect(this);
		sound->setSource(m_sounds.first()->source());
		m_sounds.append(sound);
	}
	sound->play();
}

//-----------------------------------------------------------------------------

void Sound::play(int name)
{
	if (!f_enabled) {
		return;
	}

	Sound* sound = f_sound_objects.value(name);
	if (sound) {
		sound->play();
	}
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
