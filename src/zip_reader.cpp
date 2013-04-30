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

#include "zip_reader.h"

#include <QFile>
#include <QStringList>
#include <QtEndian>

#define ZIP_DISABLE_DEPRECATED 1
#include <zip.h>

//-----------------------------------------------------------------------------

ZipReader::ZipReader(const QString& filename)
{
	m_archive = zip_open(QFile::encodeName(filename), 0, 0);
	if (m_archive) {
		readFileInfo();
	}
}

//-----------------------------------------------------------------------------

ZipReader::ZipReader(QIODevice* device) :
	m_archive(0)
{
	QFile* file = qobject_cast<QFile*>(device);
	if (file) {
		m_archive = zip_fdopen(file->handle(), 0, 0);
		if (m_archive) {
			readFileInfo();
		}
	}
}

//-----------------------------------------------------------------------------

ZipReader::~ZipReader()
{
	close();
}

//-----------------------------------------------------------------------------

void ZipReader::close()
{
	if (m_archive) {
		zip_discard(m_archive);
		m_archive = 0;
	}
}

//-----------------------------------------------------------------------------

QStringList ZipReader::fileList() const
{
	return m_files.keys();
}

//-----------------------------------------------------------------------------

QByteArray ZipReader::fileData(const QString& filename) const
{
	if (!m_files.contains(filename)) {
		return QByteArray();
	}

	zip_uint64_t pos = m_files[filename];
	zip_file* zfile = zip_fopen_index(m_archive, pos, 0);
	if (zfile == 0) {
		return QByteArray();
	}

	QByteArray result;
	char buffer[0x4000];
	int len = 0;
	while ((len = zip_fread(zfile, buffer, 0x4000)) != -1) {
		result.append(buffer, len);
		if (len < 0x4000) {
			break;
		}
	}

	zip_fclose(zfile);

	return result;
}

//-----------------------------------------------------------------------------

bool ZipReader::canRead(QIODevice* device)
{
	return qFromLittleEndian<int>((const uchar*)device->peek(4).constData()) == 0x04034b50;
}

//-----------------------------------------------------------------------------

void ZipReader::readFileInfo()
{
	zip_int64_t count = zip_get_num_entries(m_archive, 0);
	for (zip_int64_t i = 0; i < count; ++i) {
		QString name = QString::fromUtf8(zip_get_name(m_archive, i, 0));
		m_files.insert(name, i);
	}
}

//-----------------------------------------------------------------------------
