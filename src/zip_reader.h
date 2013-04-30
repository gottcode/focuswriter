/***********************************************************************
 *
 * Copyright (C) 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef ZIP_READER_H
#define ZIP_READER_H

struct zip;

#include <QHash>
#include <QString>
class QIODevice;
class QStringList;

class ZipReader
{
public:
	ZipReader(const QString& filename);
	ZipReader(QIODevice* device);
	~ZipReader();

	bool isReadable() const
	{
		return m_archive;
	}

	void close();

	QStringList fileList() const;
	QByteArray fileData(const QString& filename) const;

	static bool canRead(QIODevice* device);

private:
	void readFileInfo();

private:
	zip* m_archive;
	QHash<QString, quint64> m_files;
};

#endif
