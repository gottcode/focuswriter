/*
	SPDX-FileCopyrightText: 2010-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

Sound::Sound(int name, const QString& filename, QObject* parent)
	: QObject(parent)
	, m_name(name)
{
	QSoundEffect* sound = new QSoundEffect(this);
	sound->setSource(QUrl::fromLocalFile(f_path + "/" + filename));
	while (sound->status() == QSoundEffect::Loading) {
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
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
	for (QSoundEffect* check : std::as_const(m_sounds)) {
		if (!check->isPlaying()) {
			sound = check;
			break;
		}
	}
	if (!sound) {
		sound = new QSoundEffect(this);
		sound->setSource(m_sounds.constFirst()->source());
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
