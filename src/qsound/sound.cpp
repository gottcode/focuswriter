/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#include <QSound>

//-----------------------------------------------------------------------------

QString Sound::m_path;

//-----------------------------------------------------------------------------

Sound::Sound(const QString& filename, QObject* parent)
	: QObject(parent),
	m_filename(m_path + "/" + filename)
{
	m_sounds.append(new QSound(m_filename, this));
}

//-----------------------------------------------------------------------------

Sound::~Sound()
{
	qDeleteAll(m_sounds);
	m_sounds.clear();
}

//-----------------------------------------------------------------------------

void Sound::play()
{
	QSound* sound = 0;
	foreach (QSound* s, m_sounds) {
		if (s->isFinished()) {
			sound = s;
			break;
		}
	}
	if (sound == 0) {
		sound = new QSound(m_filename, this);
		m_sounds.append(sound);
	}
	sound->play();
}

//-----------------------------------------------------------------------------

void Sound::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------
