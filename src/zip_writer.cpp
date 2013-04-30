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

#include "zip_writer.h"

#include <QFile>
#include <QString>

#define ZIP_DISABLE_DEPRECATED 1
#include <zip.h>

//-----------------------------------------------------------------------------

ZipWriter::ZipWriter(const QString& filename)
{
	m_archive = zip_open(QFile::encodeName(filename), ZIP_CREATE | ZIP_TRUNCATE, 0);
}

//-----------------------------------------------------------------------------

ZipWriter::~ZipWriter()
{
	close();
}

//-----------------------------------------------------------------------------

bool ZipWriter::addFile(const QString& filename, const QByteArray& data, bool compressed)
{
	if (!m_archive) {
		return false;
	}

	zip_source* buffer = zip_source_buffer(m_archive, data.constData(), data.size(), 0);
	if (!buffer) {
		return false;
	}

	zip_int64_t index = zip_file_add(m_archive, filename.toUtf8(), buffer, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
	if (index == -1) {
		zip_source_free(buffer);
		return false;
	}

	if (zip_set_file_compression(m_archive, index, compressed ? ZIP_CM_DEFLATE : ZIP_CM_STORE, 0) == -1) {
		return false;
	}

	m_files.insert(filename, data);

	return true;
}

//-----------------------------------------------------------------------------

bool ZipWriter::close()
{
	bool success = true;
	if (m_archive) {
		success = (zip_close(m_archive) == 0);
		m_archive = 0;
	}
	return success;
}

//-----------------------------------------------------------------------------
