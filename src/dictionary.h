/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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
#include <QSharedPointer>
class DictionaryPrivate;

class Dictionary : public QObject
{
	Q_OBJECT

public:
	Dictionary(QObject* parent = 0);
	Dictionary(const QString& language, QObject* parent = 0);
	~Dictionary();

	QStringRef check(const QString& string, int start_at = 0) const;
	QStringList suggestions(const QString& word) const;

	void setLanguage(const QString& language);

	static QStringList availableLanguages();
	static QString defaultLanguage();
	static QString path();
	static QStringList personal();

	static void add(const QString& word);
	static void setDefaultLanguage(const QString& language);
	static void setIgnoreNumbers(bool ignore);
	static void setIgnoreUppercase(bool ignore);
	static void setPath(const QString& path);
	static void setPersonal(const QStringList& words);

signals:
	void changed();

private:
	QSharedPointer<DictionaryPrivate> d;
};

#endif
