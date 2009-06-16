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

#include "dictionary.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>

#include <hunspell.hxx>

/*****************************************************************************/

namespace {
	QList<Dictionary*> f_instances;
	Hunspell* f_dictionary = 0;
	int f_ref_count = 0;
	QString f_path;
	QString f_language;
	QStringList f_personal;
	bool f_ignore_uppercase = true;
	bool f_ignore_numbers = true;

	bool compareWords(const QString& s1, const QString& s2) {
		return s1.localeAwareCompare(s2) < 0;
	}
}

/*****************************************************************************/

Dictionary::Dictionary(QObject* parent)
: QObject(parent) {
	increment();
}

/*****************************************************************************/

Dictionary::Dictionary(const Dictionary& dictionary)
: QObject(dictionary.parent()) {
	increment();
}

/*****************************************************************************/

Dictionary& Dictionary::operator=(const Dictionary& dictionary) {
	setParent(dictionary.parent());
	increment();
	return *this;
}

/*****************************************************************************/

Dictionary::~Dictionary() {
	decrement();
}

/*****************************************************************************/

void Dictionary::add(const QString& word) {
	QStringList words = personal();
	words.append(word);
	setPersonal(words);
	foreach (Dictionary* dictionary, f_instances) {
		emit dictionary->changed();
	}
}

/*****************************************************************************/

QList<Word> Dictionary::check(const QString& string, int start_at) const {
	QList<Word> result;
	if (!f_dictionary) {
		return result;
	}

	QRegExp exp("(\\w['\\x2019\\w]+\\w)");
	int length = 0;
	while ((start_at = exp.indexIn(string, start_at + length)) != -1) {
		length = exp.matchedLength();
		QString check = string.mid(start_at, length);

		bool number = false;
		bool uppercase = f_ignore_uppercase;
		for (int i = 0; i < length; ++i) {
			QChar c = check.at(i);
			if (c.isNumber()) {
				number = true;
			} else if (c.isLower()) {
				uppercase = false;
			}
		}

		if (!uppercase && !(number && f_ignore_numbers) && !f_dictionary->spell(check.toUtf8().data())) {
			result.append(Word(start_at, length));
		}
	}

	return result;
}

/*****************************************************************************/

QStringList Dictionary::suggestions(const QString& word) const {
	QStringList result;
	if (!f_dictionary) {
		return result;
	}

	char** suggestions = 0;
	int count = f_dictionary->suggest(&suggestions, word.toUtf8().data());
	if (suggestions != 0) {
		for (int i = 0; i < count; ++i) {
			result.append(QString::fromUtf8(suggestions[i]));
		}
		f_dictionary->free_list(&suggestions, count);
	}
	return result;
}

/*****************************************************************************/

QStringList Dictionary::availableLanguages() {
	QStringList result;
	QStringList locations = QDir::searchPaths("dict");
	QListIterator<QString> i(locations);
	while (i.hasNext()) {
		QDir dir(i.next());

		QStringList dic_files = dir.entryList(QStringList() << "*.dic", QDir::Files, QDir::Name | QDir::IgnoreCase);
		dic_files.replaceInStrings(".dic", "");
		QStringList aff_files = dir.entryList(QStringList() << "*.aff", QDir::Files);
		aff_files.replaceInStrings(".aff", "");

		foreach (const QString& language, dic_files) {
			if (aff_files.contains(language) && !result.contains(language)) {
				result.append(language);
			}
		}
	}
	return result;
}

/*****************************************************************************/

QString Dictionary::language() {
	return f_language;
}

/*****************************************************************************/

QStringList Dictionary::personal() {
	return f_personal;
}

/*****************************************************************************/

void Dictionary::setIgnoreNumbers(bool ignore) {
	f_ignore_numbers = ignore;
}

/*****************************************************************************/

void Dictionary::setIgnoreUppercase(bool ignore) {
	f_ignore_uppercase = ignore;
}

/*****************************************************************************/

void Dictionary::setLanguage(const QString& language) {
	if (language.isEmpty() || language == f_language) {
		return;
	}
	f_language = language;

	delete f_dictionary;
	QString aff = QFileInfo("dict:" + language + ".aff").canonicalFilePath();
	QString dic = QFileInfo("dict:" + language + ".dic").canonicalFilePath();
	f_dictionary = new Hunspell(aff.toUtf8().data(), dic.toUtf8().data());

	QStringList words = personal();
	foreach (const QString& word, words) {
		f_dictionary->add(word.toUtf8().data());
	}

	foreach (Dictionary* dictionary, f_instances) {
		emit dictionary->changed();
	}
}

/*****************************************************************************/

void Dictionary::setPersonal(const QStringList& words) {
	if (f_dictionary) {
		foreach (const QString& word, f_personal) {
			f_dictionary->remove(word.toUtf8().data());
		}
	}

	f_personal = words;
	qSort(f_personal.begin(), f_personal.end(), compareWords);

	QFile file(f_path + "/personal");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec(QTextCodec::codecForName("UTF-8"));
		foreach (const QString& word, f_personal) {
			stream << word << "\n";
		}
	}

	if (f_dictionary) {
		foreach (const QString& word, f_personal) {
			f_dictionary->add(word.toUtf8().data());
		}
	}

	foreach (Dictionary* dictionary, f_instances) {
		emit dictionary->changed();
	}
}

/*****************************************************************************/

QString Dictionary::path() {
	return f_path;
}

/*****************************************************************************/

void Dictionary::setPath(const QString& path) {
	f_path = path;
}

/*****************************************************************************/

void Dictionary::increment() {
	f_ref_count++;
	f_instances.append(this);
	if (f_personal.isEmpty()) {
		QStringList words;
		QFile file(f_path + "/personal");
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			while (!stream.atEnd()) {
				words.append(stream.readLine());
			}
			qSort(f_personal.begin(), f_personal.end(), compareWords);
			f_personal = words;
		}
	}
}

/*****************************************************************************/

void Dictionary::decrement() {
	f_ref_count--;
	f_instances.removeOne(this);
	if (f_ref_count <= 0) {
		f_ref_count = 0;
		delete f_dictionary;
		f_dictionary = 0;
	}
}

/*****************************************************************************/
