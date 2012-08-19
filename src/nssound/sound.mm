/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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
#include <QVector>

#import <AppKit/NSSound.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSData.h>

//-----------------------------------------------------------------------------

namespace
{
	QString f_path;
	bool f_enabled = false;

	QVector<NSData*> f_chunks;
	int f_total_sounds = 0;
	QHash<QString, int> f_ids;
	QHash<int, Sound*> f_sound_objects;

	NSData* loadSound(const QString& filename)
	{
		const NSString* nsstring = [[NSString alloc] initWithCharacters:reinterpret_cast<const unichar*>(filename.unicode()) length:filename.length()];
		NSData* chunk = [[NSData alloc] initWithContentsOfFile:nsstring];
		[nsstring release];
		return chunk;
	}

	void unloadSounds()
	{
		for (int i = 0, count = f_chunks.count(); i < count; ++i) {
			[f_chunks[i] release];
		}
		f_chunks.clear();
	}
}

//-----------------------------------------------------------------------------

Sound::Sound(int name, const QString& filename, QObject* parent) :
	QObject(parent),
	m_id(-1),
	m_name(name)
{
	f_total_sounds++;

	if (f_ids.contains(filename)) {
		m_id = f_ids.value(filename);
	} else {
		NSData* chunk = loadSound(f_path + "/" + filename);
		if (chunk == nil) {
			qWarning("Unable to load sound file '%s'.", qPrintable(filename));
			return;
		}

		m_id = f_chunks.count();
		f_chunks.append(chunk);
		f_ids[filename] = m_id;
	}

	f_sound_objects[m_name] = this;
}

//-----------------------------------------------------------------------------

Sound::~Sound()
{
	f_sound_objects[m_name] = 0;
	f_total_sounds--;
	if (f_total_sounds == 0) {
		unloadSounds();
		f_ids.clear();
	}
}

//-----------------------------------------------------------------------------

void Sound::play(int name)
{
	Sound* sound = f_sound_objects.value(name);
	if (!f_enabled || !sound) {
		return;
	}

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSSound* const nssound = [[NSSound alloc] initWithData:f_chunks.at(sound->m_id)];
	[nssound play];
	[pool release];
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
