/***********************************************************************
 *
 * Copyright (C) 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "docx_writer.h"
#include "odt_writer.h"
#include "rtf_writer.h"

#include <QFile>
#include <QTextDocument>
#include <QTextStream>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <io.h>
#endif

//-----------------------------------------------------------------------------

DocumentWriter::DocumentWriter() :
	m_type("odt"),
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

	if (m_type == "odt") {
		OdtWriter writer;
		saved = writer.write(m_filename, m_document);
	} else if (m_type == "docx") {
		DocxWriter writer;
		saved = writer.write(m_filename, m_document);
	} else {
		QFile file(m_filename);
		if (file.open(QFile::WriteOnly | QFile::Truncate)) {
			if (m_type == "rtf") {
				RtfWriter writer(m_codepage);
				if (m_codepage.isEmpty()) {
					m_codepage = writer.codePage();
				}
				saved = writer.write(&file, m_document);
			} else {
				QTextStream stream(&file);
				stream.setCodec(!m_codepage.isEmpty() ? m_codepage : "UTF-8");
				if ((m_type == "txt") || (m_codepage != "UTF-8")) {
					stream.setGenerateByteOrderMark(true);
				}
				stream << m_document->toPlainText();
				saved = true;
			}

#if defined(Q_OS_UNIX)
			saved &= (fsync(file.handle()) == 0);
#elif defined(Q_OS_WIN)
			saved &= (FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(file.handle()))) != 0);
#endif
			saved &= (file.error() == QFile::NoError);
			file.close();
		}
	}

	return saved;
}

//-----------------------------------------------------------------------------
