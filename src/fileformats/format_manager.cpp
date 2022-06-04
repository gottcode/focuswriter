/*
	SPDX-FileCopyrightText: 2013-2015 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "format_manager.h"

#include "docx_reader.h"
#include "odt_reader.h"
#include "rtf_reader.h"
#include "txt_reader.h"

//-----------------------------------------------------------------------------

FormatReader* FormatManager::createReader(QIODevice* device, const QString& type)
{
	FormatReader* reader = nullptr;

	// Try file extension first
	if (type == "odt" || type == "fodt") {
		if (OdtReader::canRead(device)) {
			reader = new OdtReader;
		}
	} else if (type == "docx") {
		if (DocxReader::canRead(device)) {
			reader = new DocxReader;
		}
	} else if (type == "rtf") {
		if (RtfReader::canRead(device)) {
			reader = new RtfReader;
		}
	}

	// Fall back to finding format from contents
	if (!reader) {
		if (OdtReader::canRead(device)) {
			reader = new OdtReader;
		} else if (DocxReader::canRead(device)) {
			reader = new DocxReader;
		} else if (RtfReader::canRead(device)) {
			reader = new RtfReader;
		} else {
			reader = new TxtReader;
		}
	}

	return reader;
}

//-----------------------------------------------------------------------------

QString FormatManager::filter(const QString& type)
{
	if (type == "odt") {
		return tr("OpenDocument Text") + QLatin1String(" (*.odt)");
	} else if (type == "fodt") {
		return tr("OpenDocument Flat XML") + QLatin1String(" (*.fodt)");
	} else if (type == "docx") {
		return tr("Office Open XML") + QLatin1String(" (*.docx)");
	} else if (type == "rtf") {
		return tr("Rich Text Format") + QLatin1String(" (*.rtf)");
	} else if ((type == "txt") || (type == "text")) {
		return tr("Plain Text") + QLatin1String(" (*.txt *.text)");
	} else {
		return QString();
	}
}

//-----------------------------------------------------------------------------

QStringList FormatManager::filters(const QString& type)
{
	static const QStringList default_filters{
		filter("odt"),
		filter("fodt"),
		filter("docx"),
		filter("rtf"),
		filter("txt"),
		(tr("All Files") + QLatin1String(" (*)"))
	};

	QStringList result = default_filters;
	if (!type.isEmpty()) {
		if (type == "fodt") {
			result.move(1, 0);
		} else if (type == "docx") {
			result.move(2, 0);
		} else if (type == "rtf") {
			result.move(3, 0);
		} else if ((type == "txt") || (type == "text")) {
			result.move(4, 0);
		} else if (type != "odt") {
			result.move(5, 0);
		}
	} else {
		result.prepend(tr("All Supported Files") + QLatin1String(" (*.docx *.fodt *.odt *.rtf *.txt *.text)"));
	}
	return result;
}

//-----------------------------------------------------------------------------

bool FormatManager::isRichText(const QString& filename)
{
	const QString type = filename.section(QLatin1Char('.'), -1).toLower();
	return (type == "odt") || (type == "fodt") || (type == "docx") || (type == "rtf");
}

//-----------------------------------------------------------------------------

QStringList FormatManager::types()
{
	static const QStringList types{ "odt", "fodt", "docx", "rtf", "txt" };
	return types;
}

//-----------------------------------------------------------------------------
