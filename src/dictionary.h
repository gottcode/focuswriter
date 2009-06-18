/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
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

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <QObject>
#include <QStringList>

class Dictionary : public QObject {
	Q_OBJECT

public:
	Dictionary(QObject* parent = 0);
	Dictionary(const Dictionary& dictionary);
	Dictionary& operator=(const Dictionary& dictionary);
	~Dictionary();

	void add(const QString& word);
	QStringRef check(const QString& string, int start_at = 0) const;
	QStringList suggestions(const QString& word) const;

	QStringList availableLanguages();
	QString language();
	QStringList personal();
	void setLanguage(const QString& language);
	void setIgnoreNumbers(bool ignore);
	void setIgnoreUppercase(bool ignore);
	void setPersonal(const QStringList& words);

	static QString path();
	static void setPath(const QString& path);

signals:
	void changed();

private:
	void increment();
	void decrement();
};

#endif
