/*
	SPDX-FileCopyrightText: 2010-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "rtf_writer.h"

#include <QHash>
#include <QIODevice>
#include <QLocale>
#include <QTextBlock>
#include <QTextCodec>
#include <QTextDocument>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <clocale>
#endif

//-----------------------------------------------------------------------------

namespace
{

#ifndef Q_OS_WIN

QByteArray fetchCodePage()
{
	// Search code page map for current language
	QLocale::Language language = QLocale().language();
	static const QHash<QLocale::Language, QByteArray> codepages{
		{ QLocale::Thai, "CP874" },
		{ QLocale::Japanese, "CP932" },
		{ QLocale::Korean, "CP949" },
		{ QLocale::Albanian, "CP1250" },
		{ QLocale::Bosnian, "CP1250" },
		{ QLocale::Croatian, "CP1250" },
		{ QLocale::Czech, "CP1250" },
		{ QLocale::Hungarian, "CP1250" },
		{ QLocale::Polish, "CP1250" },
		{ QLocale::Romanian, "CP1250" },
		{ QLocale::Serbian, "CP1250" },
		{ QLocale::Slovak, "CP1250" },
		{ QLocale::Slovenian, "CP1250" },
		{ QLocale::Turkmen, "CP1250" },
		{ QLocale::Bashkir, "CP1251" },
		{ QLocale::Bulgarian, "CP1251" },
		{ QLocale::Byelorussian, "CP1251" },
		{ QLocale::Kazakh, "CP1251" },
		{ QLocale::Kirghiz, "CP1251" },
		{ QLocale::Macedonian, "CP1251" },
		{ QLocale::Mongolian, "CP1251" },
		{ QLocale::Russian, "CP1251" },
		{ QLocale::Tajik, "CP1251" },
		{ QLocale::Tatar, "CP1251" },
		{ QLocale::Ukrainian, "CP1251" },
		{ QLocale::Afrikaans, "CP1252" },
		{ QLocale::Basque, "CP1252" },
		{ QLocale::Breton, "CP1252" },
		{ QLocale::CentralMoroccoTamazight, "CP1252" },
		{ QLocale::Corsican, "CP1252" },
		{ QLocale::Danish, "CP1252" },
		{ QLocale::Dutch, "CP1252" },
		{ QLocale::English, "CP1252" },
		{ QLocale::Faroese, "CP1252" },
		{ QLocale::Filipino, "CP1252" },
		{ QLocale::Finnish, "CP1252" },
		{ QLocale::French, "CP1252" },
		{ QLocale::Frisian, "CP1252" },
		{ QLocale::Galician, "CP1252" },
		{ QLocale::Gaelic, "CP1252" },
		{ QLocale::German, "CP1252" },
		{ QLocale::Greenlandic, "CP1252" },
		{ QLocale::Hausa, "CP1252" },
		{ QLocale::Icelandic, "CP1252" },
		{ QLocale::Igbo, "CP1252" },
		{ QLocale::Indonesian, "CP1252" },
		{ QLocale::Inuktitut, "CP1252" },
		{ QLocale::Irish, "CP1252" },
		{ QLocale::Italian, "CP1252" },
		{ QLocale::Kinyarwanda, "CP1252" },
		{ QLocale::LowGerman, "CP1252" },
		{ QLocale::Malay, "CP1252" },
		{ QLocale::NorthernSami, "CP1252" },
		{ QLocale::NorwegianBokmal, "CP1252" },
		{ QLocale::NorwegianNynorsk, "CP1252" },
		{ QLocale::Occitan, "CP1252" },
		{ QLocale::Portuguese, "CP1252" },
		{ QLocale::RhaetoRomance, "CP1252" },
		{ QLocale::Quechua, "CP1252" },
		{ QLocale::Spanish, "CP1252" },
		{ QLocale::Swahili, "CP1252" },
		{ QLocale::Swedish, "CP1252" },
		{ QLocale::SwissGerman, "CP1252" },
		{ QLocale::Welsh, "CP1252" },
		{ QLocale::Wolof, "CP1252" },
		{ QLocale::Xhosa, "CP1252" },
		{ QLocale::Yoruba, "CP1252" },
		{ QLocale::Zulu, "CP1252" },
		{ QLocale::Greek, "CP1253" },
		{ QLocale::Azerbaijani, "CP1254" },
		{ QLocale::Turkish, "CP1254" },
		{ QLocale::Uzbek, "CP1254" },
		{ QLocale::Hebrew, "CP1255" },
		{ QLocale::Yiddish, "CP1255" },
		{ QLocale::Arabic, "CP1256" },
		{ QLocale::Persian, "CP1256" },
		{ QLocale::Urdu, "CP1256" },
		{ QLocale::Estonian, "CP1257" },
		{ QLocale::Latvian, "CP1257" },
		{ QLocale::Lithuanian, "CP1257" },
		{ QLocale::Vietnamese, "CP1258" }
	};
	QByteArray codepage = codepages.value(language);

	// Guess at Chinese code page for current country
	if (codepage.isEmpty() && language == QLocale::Chinese) {
		const QLocale::Country country = QLocale().country();
		codepage = (country == QLocale::HongKong || country == QLocale::Macau || country == QLocale::Taiwan) ? "CP950" : "CP936";
	}

	// Guess at closest code page from environment variables
	if (codepage.isEmpty()) {
		QByteArray lang = setlocale(LC_CTYPE, 0);
		if (lang.isEmpty() || lang == "C") {
			lang = qgetenv("LC_ALL");
		}
		if (lang.isEmpty() || lang == "C") {
			lang = qgetenv("LC_CTYPE");
		}
		if (lang.isEmpty() || lang == "C") {
			lang = qgetenv("LANG");
		}

		if (language == QLocale::Serbian) {
			codepage = (lang.contains("@latin") || lang.contains("Latn")) ? "CP1250" : "CP1251";
		}

		if (lang.contains("8859-2")) {
			codepage = "CP1250";
		} else if (lang.contains("8859-3")) {
			codepage = "CP1254";
		} else if (lang.contains("8859-4")) {
			codepage = "CP1257";
		} else if (lang.contains("8859-5")) {
			codepage = "CP1251";
		} else if (lang.contains("8859-6")) {
			codepage = "CP1256";
		} else if (lang.contains("8859-7")) {
			codepage = "CP1253";
		} else if (lang.contains("8859-8")) {
			codepage = "CP1255";
		} else if (lang.contains("8859-9")) {
			codepage = "CP1254";
		} else if (lang.contains("8859-11")) {
			codepage = "CP874";
		} else if (lang.contains("8859-13")) {
			codepage = "CP1257";
		}

		if (codepage.isEmpty() && lang.contains("UTF-8")) {
			codepage = "CP65001";
		}
	}

	// Fall back to Western European
	if (codepage.isEmpty()) {
		codepage = "CP1252";
	}

	return codepage;
}

#else

QByteArray fetchCodePage()
{
	TCHAR buffer[7];
	const int size = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, buffer, sizeof(buffer) / sizeof(TCHAR));
#if UNICODE
	const QString codepage = QLatin1String("CP") + QString::fromWCharArray(buffer, size - 1);
#else
	const QString codepage = QLatin1String("CP") + QString::fromLocal8Bit(buffer);
#endif
	return codepage.toLatin1();
}

#endif

}

//-----------------------------------------------------------------------------

RtfWriter::RtfWriter(const QByteArray& encoding)
	: m_encoding(encoding)
	, m_codec(nullptr)
	, m_supports_ascii(false)
{
	// Fetch system codepage
	if (m_encoding.isEmpty()) {
		m_encoding = fetchCodePage();
	}
	if (m_encoding == "CP932") {
		m_encoding = "Shift-JIS";
	}

	// Load codec
	m_codec = QTextCodec::codecForName(m_encoding);
	if (!m_codec) {
		m_encoding = "CP1252";
		m_codec = QTextCodec::codecForName(m_encoding);
	}

	// Check if codec is a superset of ASCII
	static QHash<int, bool> supports_ascii;
	const int mib = m_codec->mibEnum();
	if (supports_ascii.contains(mib)) {
		m_supports_ascii = supports_ascii[mib];
	} else {
		m_supports_ascii = true;
		QByteArray encoded;
		QTextCodec::ConverterState state;
		state.flags = QTextCodec::ConvertInvalidToNull;
		for (int i = 0x20; i < 0x80; ++i) {
			const QChar c = QChar::fromLatin1(i);
			encoded = m_codec->fromUnicode(&c, 1, &state);
			if (state.invalidChars || (encoded.length() > 1) || (encoded.at(0) != i)) {
				m_supports_ascii = false;
				break;
			}
		}
		supports_ascii.insert(mib, m_supports_ascii);
	}

	// Create header
	switch (m_codec->mibEnum()) {
	case -168: m_header = "{\\rtf1\\mac\\ansicpg10000\n"; break;
	case 17: m_header = "{\\rtf1\\ansi\\ansicpg932\n"; break;
	case 106: m_header = "{\\rtf1\\ansi\\ansicpg65001\n"; break;
	case 2009: m_header = "{\\rtf1\\pca\\ansicpg850\n"; break;
	default: m_header = "{\\rtf1\\ansi\\ansicpg" + m_encoding.mid(2) + "\n"; break;
	}
	m_header += "{\\stylesheet\n"
		"{\\s0 Normal;}\n"
		"{\\s1\\snext0\\outlinelevel0\\fs36\\b Heading 1;}\n"
		"{\\s2\\snext0\\outlinelevel1\\fs32\\b Heading 2;}\n"
		"{\\s3\\snext0\\outlinelevel2\\fs28\\b Heading 3;}\n"
		"{\\s4\\snext0\\outlinelevel3\\fs24\\b Heading 4;}\n"
		"{\\s5\\snext0\\outlinelevel4\\fs20\\b Heading 5;}\n"
		"{\\s6\\snext0\\outlinelevel5\\fs16\\b Heading 6;}\n"
		"}\n";
}

//-----------------------------------------------------------------------------

bool RtfWriter::write(QIODevice* device, const QTextDocument* text)
{
	if (!m_codec) {
		return false;
	}

	device->write(m_header);

	for (QTextBlock block = text->begin(); block.isValid(); block = block.next()) {
		QByteArray par("{\\pard\\plain");
		const QTextBlockFormat block_format = block.blockFormat();
		const int heading = block_format.property(QTextFormat::UserProperty).toInt();
		if (heading) {
			par += "\\s" + QByteArray::number(heading)
				+ "\\outlinelevel" + QByteArray::number(heading - 1)
				+ "\\fs" + QByteArray::number((10 - heading) * 4)
				+ "\\b";
		} else {
			par += "\\s0";
		}
		const bool rtl = block_format.layoutDirection() == Qt::RightToLeft;
		if (rtl) {
			par += "\\rtlpar";
		}
		const Qt::Alignment align = block_format.alignment();
		if (rtl && (align & Qt::AlignLeft)) {
			par += "\\ql";
		} else if (align & Qt::AlignRight) {
			par += "\\qr";
		} else if (align & Qt::AlignCenter) {
			par += "\\qc";
		} else if (align & Qt::AlignJustify) {
			par += "\\qj";
		}
		if (block_format.indent() > 0) {
			par += "\\li" + QByteArray::number(block_format.indent() * 720);
		}
		device->write(par);

		if (block.begin() != block.end()) {
			device->write(" ");
			for (QTextBlock::iterator iter = block.begin(); !iter.atEnd(); ++iter) {
				const QTextFragment fragment = iter.fragment();
				const QTextCharFormat char_format = fragment.charFormat();
				QByteArray style;
				if (char_format.fontWeight() == QFont::Bold) {
					style += "\\b";
				}
				if (char_format.fontItalic()) {
					style += "\\i";
				}
				if (char_format.fontUnderline()) {
					style += "\\ul";
				}
				if (char_format.fontStrikeOut()) {
					style += "\\strike";
				}
				if (char_format.verticalAlignment() == QTextCharFormat::AlignSuperScript) {
					style += "\\super";
				} else if (char_format.verticalAlignment() == QTextCharFormat::AlignSubScript) {
					style += "\\sub";
				}

				if (!style.isEmpty()) {
					device->write("{" + style + " " + fromUnicode(fragment.text()) + "}");
				} else {
					device->write(fromUnicode(fragment.text()));
				}
			}
		}

		device->write("\\par}\n");
	}

	device->write("}");
	return true;
}

//-----------------------------------------------------------------------------

QByteArray RtfWriter::fromUnicode(const QString& string) const
{
	QByteArray text;

	QTextCodec::ConverterState state;
	state.flags = QTextCodec::ConvertInvalidToNull;

	for (const QChar& c : string) {
		switch (c.unicode()) {
		case '\t': text += "\\tab "; break;
		case '\\': text += "\\'5C"; break;
		case '{': text += "\\'7B"; break;
		case '}': text += "\\'7D"; break;
		case 0x00a0: text += "\\~"; break;
		case 0x00ad: text += "\\-"; break;
		case 0x00b7: text += "\\|"; break;
		case 0x2002: text += "\\enspace "; break;
		case 0x2003: text += "\\emspace "; break;
		case 0x2004: text += "\\qmspace "; break;
		case 0x200c: text += "\\zwnj "; break;
		case 0x200d: text += "\\zwj "; break;
		case 0x200e: text += "\\ltrmark "; break;
		case 0x200f: text += "\\rtlmark "; break;
		case 0x2011: text += "\\_"; break;
		case 0x2013: text += "\\endash "; break;
		case 0x2014: text += "\\emdash "; break;
		case 0x2018: text += "\\lquote "; break;
		case 0x2019: text += "\\rquote "; break;
		case 0x201c: text += "\\ldblquote "; break;
		case 0x201d: text += "\\rdblquote "; break;
		case 0x2022: text += "\\bullet "; break;
		case 0x2028: text += "\\line "; break;
		default:
			if (m_supports_ascii && (c.unicode() >= 0x0020) && (c.unicode() < 0x0080)) {
				text += c.unicode();
				break;
			}

			const QByteArray encoded = m_codec->fromUnicode(&c, 1, &state);
			if ((state.invalidChars == 0) && (encoded.at(0) != 0)) {
				if (encoded.length() == 1 && encoded.at(0) >= 0x20) {
					text += encoded;
				} else {
					text += "\\'" + encoded.toHex().toUpper();
				}
			} else if (c.unicode()) {
				text += "\\u" + QByteArray::number(c.unicode()) + "?";
			}
		}
	}

	return text;
}

//-----------------------------------------------------------------------------
