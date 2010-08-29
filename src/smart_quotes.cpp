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

#include "smart_quotes.h"

#include "preferences.h"

#include <QKeyEvent>
#include <QLocale>
#include <QProgressDialog>
#include <QTextCursor>
#include <QTextEdit>

//-----------------------------------------------------------------------------

bool SmartQuotes::m_enabled = true;
QChar SmartQuotes::m_quotes[4] = { 0x201c, 0x201d, 0x2018, 0x2019 };
const SmartQuotes::Quotes SmartQuotes::m_quotes_list[] = {
	{ 0x201c, 0x201d },
	{ 0x2018, 0x2019 },
	{ 0x201e, 0x201c },
	{ 0x201a, 0x2018 },
	{ 0x201e, 0x201d },
	{ 0x201a, 0x2019 },
	{ 0x201c, 0x201e },
	{ 0x2018, 0x201a },
	{ 0x201d, 0x201d },
	{ 0x2019, 0x2019 },
	{ 0x201d, 0x201c },
	{ 0x2019, 0x2018 },
	{ 0x00ab, 0x00bb },
	{ 0x2039, 0x203a },
	{ 0x00bb, 0x00ab },
	{ 0x203a, 0x2039 },
	{ 0x00bb, 0x00bb },
	{ 0x203a, 0x203a },
	{ 0x300c, 0x300d },
	{ 0x300e, 0x300f },
	{ 0x0022, 0x0022 },
	{ 0x0027, 0x0027 }
};
const size_t SmartQuotes::m_quotes_list_count = sizeof(m_quotes_list) / sizeof(Quotes);

//-----------------------------------------------------------------------------

bool SmartQuotes::insert(QTextEdit* text, QKeyEvent* key)
{
	int quote = 2;
	if (key->key() == Qt::Key_QuoteDbl) {
		quote = 0;
	} else if (key->key() != Qt::Key_Apostrophe) {
		return false;
	}

	QTextCursor cursor = text->textCursor();
	QChar c = text->document()->characterAt(cursor.position() - 1);
	if (!c.isSpace() && !c.isNull()) {
		quote++;
	}

	cursor.beginEditBlock();
	cursor.insertText(key->text());
	cursor.endEditBlock();

	if (key->text().right(1) != m_quotes[quote]) {
		cursor.beginEditBlock();
		cursor.deletePreviousChar();
		cursor.insertText(m_quotes[quote]);
		cursor.endEditBlock();
	}

	return true;
}

//-----------------------------------------------------------------------------

void SmartQuotes::replace(QTextEdit* text, int start, int end)
{
	QProgressDialog progress(text);
	progress.setCancelButton(0);
	progress.setLabelText(tr("Replacing quotation marks..."));
	progress.setWindowTitle(tr("Please Wait"));
	progress.setModal(true);
	progress.setMinimum(start);
	progress.setMaximum(end);
	progress.setMinimumDuration(0);

	QTextCursor cursor(text->document());
	cursor.beginEditBlock();
	QChar previous = text->document()->characterAt(start - 1);
	for (int i = start; i < end; ++i) {
		QChar c = text->document()->characterAt(i);
		int quote = 2;
		if (c == '"') {
			quote = 0;
		} else if (c != '\'') {
			previous = c;
			continue;
		}

		if (!previous.isSpace() && !previous.isNull()) {
			quote++;
		}
		previous = c;

		if (c != m_quotes[quote]) {
			cursor.setPosition(i);
			cursor.deleteChar();
			cursor.insertText(m_quotes[quote]);
		}

		progress.setValue(i);
	}
	cursor.endEditBlock();
}

//-----------------------------------------------------------------------------

QString SmartQuotes::quoteString(const QString& string, size_t index)
{
	QString result = string;
	if (index < count()) {
		const Quotes& quotes = m_quotes_list[index];
		result.prepend(quotes.left);
		result.append(quotes.right);
	}
	return result;
}

//-----------------------------------------------------------------------------

namespace
{
	struct DefaultQuotes
	{
		QLocale::Language language;
		QLocale::Country country;
		size_t double_index;
		size_t single_index;
	};
}

void SmartQuotes::loadPreferences(Preferences& preferences)
{
	m_enabled = preferences.smartQuotes();
	size_t double_index = preferences.doubleQuotes();
	size_t single_index = preferences.singleQuotes();
	if (double_index < count() && single_index < count()) {
		setQuotes(double_index, single_index);
		return;
	}

	const DefaultQuotes default_quotes[] = {
		{ QLocale::Afrikaans,    QLocale::AnyCountry,   0,  1 },
		{ QLocale::Albanian,     QLocale::AnyCountry,   2,  1 },
		{ QLocale::Basque,       QLocale::AnyCountry,  12, 13 },
		{ QLocale::Bulgarian,    QLocale::AnyCountry,   2, 21 },
		{ QLocale::Byelorussian, QLocale::AnyCountry,  12, 21 },
		{ QLocale::Catalan,      QLocale::AnyCountry,  12,  0 },
		{ QLocale::Chinese,      QLocale::AnyCountry,  18, 19 },
		{ QLocale::Croatian,     QLocale::AnyCountry,   4,  5 },
		{ QLocale::Czech,        QLocale::AnyCountry,   2,  3 },
		{ QLocale::Danish,       QLocale::AnyCountry,  14, 15 },
		{ QLocale::Dutch,        QLocale::AnyCountry,   0,  1 },
		{ QLocale::English,      QLocale::AnyCountry,   0,  1 },
		{ QLocale::Esperanto,    QLocale::AnyCountry,   0,  1 },
		{ QLocale::Estonian,     QLocale::AnyCountry,   2, 21 },
		{ QLocale::Finnish,      QLocale::AnyCountry,   8,  9 },
		{ QLocale::French,       QLocale::Switzerland, 12, 13 },
		{ QLocale::French,       QLocale::AnyCountry,  12,  0 },
		{ QLocale::Georgian,     QLocale::AnyCountry,   2, 21 },
		{ QLocale::German,       QLocale::Switzerland, 12, 13 },
		{ QLocale::German,       QLocale::AnyCountry,   2,  3 },
		{ QLocale::Greek,        QLocale::AnyCountry,  12, 21 },
		{ QLocale::Hebrew,       QLocale::AnyCountry,   0, 21 },
		{ QLocale::Hungarian,    QLocale::AnyCountry,   4, 14 },
		{ QLocale::Icelandic,    QLocale::AnyCountry,   2,  3 },
		{ QLocale::Indonesian,   QLocale::AnyCountry,   0,  1 },
		{ QLocale::Irish,        QLocale::AnyCountry,   0,  1 },
		{ QLocale::Italian,      QLocale::Switzerland, 12, 13 },
		{ QLocale::Italian,      QLocale::AnyCountry,  12, 21 },
		{ QLocale::Japanese,     QLocale::AnyCountry,  18, 19 },
		{ QLocale::Korean,       QLocale::AnyCountry,   0,  1 },
		{ QLocale::Latvian,      QLocale::AnyCountry,  12,  2 },
		{ QLocale::Lithuanian,   QLocale::AnyCountry,   2,  3 },
		{ QLocale::Macedonian,   QLocale::AnyCountry,   2, 21 },
		{ QLocale::Norwegian,    QLocale::AnyCountry,   2,  9 },
		{ QLocale::Polish,       QLocale::AnyCountry,   4, 12 },
		{ QLocale::Portuguese,   QLocale::Brazil,       0,  1 },
		{ QLocale::Portuguese,   QLocale::AnyCountry,  12,  0 },
		{ QLocale::Romanian,     QLocale::AnyCountry,   4, 12 },
		{ QLocale::Russian,      QLocale::AnyCountry,  12,  2 },
		{ QLocale::Serbian,      QLocale::AnyCountry,   2,  9 },
		{ QLocale::Slovak,       QLocale::AnyCountry,   2,  3 },
		{ QLocale::Slovenian,    QLocale::AnyCountry,   2,  3 },
		{ QLocale::Spanish,      QLocale::AnyCountry,  12,  0 },
		{ QLocale::Swedish,      QLocale::AnyCountry,   8,  9 },
		{ QLocale::Thai,         QLocale::AnyCountry,   0,  1 },
		{ QLocale::Turkish,      QLocale::AnyCountry,  12, 13 },
		{ QLocale::Ukrainian,    QLocale::AnyCountry,  12, 21 },
		{ QLocale::Welsh,        QLocale::AnyCountry,   0,  1 }
	};
	const size_t default_quotes_count = sizeof(default_quotes) / sizeof(DefaultQuotes);

	QLocale locale = QLocale::system();
	double_index = 0;
	single_index = 1;
	for (size_t i = 0; i < default_quotes_count; ++i) {
		const DefaultQuotes& quotes = default_quotes[i];
		if (quotes.language == locale.language() && (quotes.country == QLocale::AnyCountry || quotes.country == locale.country())) {
			double_index = quotes.double_index;
			single_index = quotes.single_index;
			break;
		}
	}

	preferences.setDoubleQuotes(double_index);
	preferences.setSingleQuotes(single_index);
	setQuotes(double_index, single_index);
}

//-----------------------------------------------------------------------------

void SmartQuotes::setQuotes(size_t index_double, size_t index_single)
{
	const Quotes& double_quotes = m_quotes_list[index_double];
	m_quotes[0] = double_quotes.left;
	m_quotes[1] = double_quotes.right;

	const Quotes& single_quotes = m_quotes_list[index_single];
	m_quotes[2] = single_quotes.left;
	m_quotes[3] = single_quotes.right;
}

//-----------------------------------------------------------------------------
