/***********************************************************************
 *
 * Copyright (C) 2012 Graeme Gott <graeme@gottcode.org>
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

#include "document_writer.h"

#include "rtf/writer.h"

#include <QFile>
#include <QTextDocument>
#include <QTextDocumentWriter>
#include <QTextStream>

#if defined(Q_OS_MAC)
#include <sys/fcntl.h>
#elif defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <io.h>
#endif

//-----------------------------------------------------------------------------

DocumentWriter::DocumentWriter() :
	m_rich_text(true),
	m_document(0)
{
}

//-----------------------------------------------------------------------------

DocumentWriter::~DocumentWriter()
{
	if (m_document && !m_document->parent()) {
		delete m_document;
	}
}

//-----------------------------------------------------------------------------

bool DocumentWriter::write()
{
	Q_ASSERT(m_document != 0);
	Q_ASSERT(!m_filename.isEmpty());

	bool saved = false;
	QFile file(m_filename + ".tmp");
	if (!m_rich_text) {
		if (file.open(QFile::WriteOnly | QFile::Text)) {
			QTextStream stream(&file);
			stream.setCodec("UTF-8");
			if (m_type == "txt") {
				stream.setGenerateByteOrderMark(true);
			}
			stream << m_document->toPlainText();
			saved = true;
		}
	} else {
		if (file.open(QFile::WriteOnly)) {
			if (m_type == "odt") {
				QTextDocumentWriter writer(&file, "ODT");
				saved = writer.write(m_document);
			} else {
				RTF::Writer writer(m_codepage);
				if (m_codepage.isEmpty()) {
					m_codepage = writer.codePage();
				}
				saved = writer.write(&file, m_document);
			}
		}
	}

	if (file.isOpen()) {
#if defined(Q_OS_MAC)
		saved &= (fcntl(file.handle(), F_FULLFSYNC, NULL) == 0);
#elif defined(Q_OS_UNIX)
		saved &= (fsync(file.handle()) == 0);
#elif defined(Q_OS_WIN)
		saved &= (FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(file.handle()))) != 0);
#endif
		saved &= (file.error() == QFile::NoError);
		file.close();
	}

	if (saved) {
		saved &= QFile::remove(m_filename);
		saved &= QFile::rename(m_filename + ".tmp", m_filename);
	}
	return saved;
}

//-----------------------------------------------------------------------------
