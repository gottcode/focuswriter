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

#ifndef SOUND_H
#define SOUND_H

#include <QObject>
class QSound;

class Sound : public QObject
{
public:
	Sound(const QString& filename, QObject* parent = 0);
	~Sound();

	void play();

	static void setPath(const QString& path);

private:
	QString m_filename;
	QList<QSound*> m_sounds;
	static QString m_path;
};

#endif
