/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2013, 2014 Graeme Gott <graeme@gottcode.org>
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

#ifndef SMART_QUOTES_H
#define SMART_QUOTES_H

#include <QChar>
#include <QCoreApplication>
class QKeyEvent;
class QLineEdit;
class QTextEdit;

class SmartQuotes
{
	Q_DECLARE_TR_FUNCTIONS(SmartQuote)

public:
	static size_t count();
	static bool isEnabled();

	static bool insert(QLineEdit* edit, QKeyEvent* key);
	static bool insert(QTextEdit* text, QKeyEvent* key);
	static void replace(QTextEdit* text, int start, int end);
	static void replace(QString& string);
	static QString revert(const QString& string);
	static QStringList revert(const QStringList& strings);
	static QString quoteString(const QString& string, size_t index);

	static void loadPreferences();

private:
	static void setQuotes(size_t index_double, size_t index_single);

private:
	static bool m_enabled;
	static QString m_quotes[4];

	struct Quotes
	{
		const QString left, right;
	};
	static const Quotes m_quotes_list[];
	static const size_t m_quotes_list_count;
};

inline size_t SmartQuotes::count()
{
	return m_quotes_list_count;
}

inline bool SmartQuotes::isEnabled()
{
	return m_enabled;
}

#endif
