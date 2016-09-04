/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2016 Graeme Gott <graeme@gottcode.org>
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
