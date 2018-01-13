/***********************************************************************
 *
 * Copyright (C) 2012, 2013, 2014, 2015, 2017, 2018 Graeme Gott <graeme@gottcode.org>
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

#include <QSaveFile>
#include <QTextDocument>
#include <QTextStream>

//-----------------------------------------------------------------------------

DocumentWriter::DocumentWriter() :
	m_type("odt"),
	m_document(0),
	m_write_bom(false)
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

	QSaveFile file(m_filename);
	file.setDirectWriteFallback(true);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}

	if (m_type == "odt") {
		OdtWriter writer;
		saved = writer.write(&file, m_document);
	} else if (m_type == "fodt") {
		OdtWriter writer;
		writer.setFlatXML(true);
		saved = writer.write(&file, m_document);
	} else if (m_type == "docx") {
		DocxWriter writer;
		saved = writer.write(&file, m_document);
	} else if (m_type == "rtf") {
		file.setTextModeEnabled(true);
		RtfWriter writer(m_encoding);
		if (m_encoding.isEmpty()) {
			m_encoding = writer.encoding();
		}
		saved = writer.write(&file, m_document);
	} else {
		file.setTextModeEnabled(true);
		QTextStream stream(&file);
		QByteArray encoding = !m_encoding.isEmpty() ? m_encoding : "UTF-8";
		stream.setCodec(encoding);
		if (m_write_bom || (encoding != "UTF-8")) {
			stream.setGenerateByteOrderMark(true);
		}
		stream << m_document->toPlainText();
		saved = stream.status() == QTextStream::Ok;
	}

	if (saved) {
		saved = file.commit();
	} else {
		file.cancelWriting();
	}

	return saved;
}

//-----------------------------------------------------------------------------
