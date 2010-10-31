/***********************************************************************
 *
 * Copyright (C) 2009, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "smart_quotes.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>

#include <hunspell.hxx>

//-----------------------------------------------------------------------------

namespace
{
	QList<Dictionary*> f_instances;
	Hunspell* f_dictionary = 0;
	QTextCodec* f_codec = 0;
	int f_ref_count = 0;
	QString f_path;
	QString f_language;
	QStringList f_personal;
	bool f_ignore_uppercase = true;
	bool f_ignore_numbers = true;

	bool compareWords(const QString& s1, const QString& s2)
	{
		return s1.localeAwareCompare(s2) < 0;
	}
}

//-----------------------------------------------------------------------------

Dictionary::Dictionary(QObject* parent)
	: QObject(parent)
{
	increment();
}

//-----------------------------------------------------------------------------

Dictionary::Dictionary(const Dictionary& dictionary)
	: QObject(dictionary.parent())
{
	increment();
}

//-----------------------------------------------------------------------------

Dictionary& Dictionary::operator=(const Dictionary& dictionary)
{
	setParent(dictionary.parent());
	increment();
	return *this;
}

//-----------------------------------------------------------------------------

Dictionary::~Dictionary()
{
	decrement();
}

//-----------------------------------------------------------------------------

void Dictionary::add(const QString& word)
{
	QStringList words = personal();
	if (words.contains(SmartQuotes::revert(word))) {
		return;
	}
	words.append(word);
	setPersonal(words);
	foreach (Dictionary* dictionary, f_instances) {
		emit dictionary->changed();
	}
}

//-----------------------------------------------------------------------------

QStringRef Dictionary::check(const QString& string, int start_at) const
{
	if (!f_dictionary) {
		return QStringRef();
	}

	int index = -1;
	int length = 0;
	int chars = 1;
	bool number = false;
	bool uppercase = f_ignore_uppercase;
	bool word = false;

	int count = string.length() - 1;
	for (int i = start_at; i <= count; ++i) {
		QChar c = string.at(i);
		switch (c.category()) {
			case QChar::Number_DecimalDigit:
			case QChar::Number_Letter:
			case QChar::Number_Other:
				number = true;
				goto Letter;
			case QChar::Letter_Lowercase:
				uppercase = false;
				Letter:
			case QChar::Letter_Uppercase:
			case QChar::Letter_Titlecase:
			case QChar::Letter_Modifier:
			case QChar::Letter_Other:
			case QChar::Mark_NonSpacing:
			case QChar::Mark_SpacingCombining:
			case QChar::Mark_Enclosing:
				if (index == -1) {
					index = i;
					chars = 1;
					length = 0;
				}
				length += chars;
				chars = 1;
				break;

			case QChar::Punctuation_FinalQuote:
			case QChar::Punctuation_Other:
				if (c == 0x0027 || c == 0x2019) {
					chars++;
					break;
				}

			default:
				if (index != -1) {
					word = true;
				}
				break;
		}

		if (word || (i == count && index != -1)) {
			if (!uppercase && !(number && f_ignore_numbers)) {
				QStringRef check(&string, index, length);
				if (!f_dictionary->spell(f_codec->fromUnicode(check.toString()).constData())) {
					return check;
				}
			}
			index = -1;
			word = false;
			number = false;
			uppercase = f_ignore_uppercase;
		}
	}
	return QStringRef();
}

//-----------------------------------------------------------------------------

QStringList Dictionary::suggestions(const QString& word) const
{
	QStringList result;
	if (!f_dictionary) {
		return result;
	}

	char** suggestions = 0;
	int count = f_dictionary->suggest(&suggestions, f_codec->fromUnicode(word).constData());
	if (suggestions != 0) {
		for (int i = 0; i < count; ++i) {
			result.append(f_codec->toUnicode(suggestions[i]));
		}
		f_dictionary->free_list(&suggestions, count);
	}
	return result;
}

//-----------------------------------------------------------------------------

QStringList Dictionary::availableLanguages()
{
	QStringList result;
	QStringList locations = QDir::searchPaths("dict");
	QListIterator<QString> i(locations);
	while (i.hasNext()) {
		QDir dir(i.next());

		QStringList dic_files = dir.entryList(QStringList() << "*.dic*", QDir::Files, QDir::Name | QDir::IgnoreCase);
		dic_files.replaceInStrings(QRegExp("\\.dic.*"), "");
		QStringList aff_files = dir.entryList(QStringList() << "*.aff*", QDir::Files);
		aff_files.replaceInStrings(QRegExp("\\.aff.*"), "");

		foreach (const QString& language, dic_files) {
			if (aff_files.contains(language) && !result.contains(language)) {
				result.append(language);
			}
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

QString Dictionary::language()
{
	return f_language;
}

//-----------------------------------------------------------------------------

QStringList Dictionary::personal()
{
	return f_personal;
}

//-----------------------------------------------------------------------------

void Dictionary::setIgnoreNumbers(bool ignore)
{
	f_ignore_numbers = ignore;
}

//-----------------------------------------------------------------------------

void Dictionary::setIgnoreUppercase(bool ignore)
{
	f_ignore_uppercase = ignore;
}

//-----------------------------------------------------------------------------

void Dictionary::setLanguage(const QString& language)
{
	QString aff = QFileInfo("dict:" + language + ".aff").canonicalFilePath();
	if (aff.isEmpty()) {
		aff = QFileInfo("dict:" + language + ".aff.hz").canonicalFilePath();
		aff.chop(3);
	}
	QString dic = QFileInfo("dict:" + language + ".dic").canonicalFilePath();
	if (dic.isEmpty()) {
		dic = QFileInfo("dict:" + language + ".dic.hz").canonicalFilePath();
		dic.chop(3);
	}
	if (language.isEmpty() || aff.isEmpty() || dic.isEmpty() || language == f_language) {
		return;
	}
	f_language = language;

	delete f_dictionary;
	f_dictionary = new Hunspell(QFile::encodeName(aff).constData(), QFile::encodeName(dic).constData());
	f_codec = QTextCodec::codecForName(f_dictionary->get_dic_encoding());

	if (f_codec) {
		QStringList words = personal();
		foreach (const QString& word, words) {
			f_dictionary->add(f_codec->fromUnicode(word).constData());
		}
	} else {
		delete f_dictionary;
		f_dictionary = 0;
	}

	foreach (Dictionary* dictionary, f_instances) {
		emit dictionary->changed();
	}
}

//-----------------------------------------------------------------------------

void Dictionary::setPersonal(const QStringList& words)
{
	if (f_dictionary) {
		foreach (const QString& word, f_personal) {
			f_dictionary->remove(f_codec->fromUnicode(word).constData());
		}
	}

	f_personal = SmartQuotes::revert(words);
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
			f_dictionary->add(f_codec->fromUnicode(word).constData());
		}
	}

	foreach (Dictionary* dictionary, f_instances) {
		emit dictionary->changed();
	}
}

//-----------------------------------------------------------------------------

QString Dictionary::path()
{
	return f_path;
}

//-----------------------------------------------------------------------------

void Dictionary::setPath(const QString& path)
{
	f_path = path;
}

//-----------------------------------------------------------------------------

void Dictionary::increment()
{
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

//-----------------------------------------------------------------------------

void Dictionary::decrement()
{
	f_ref_count--;
	f_instances.removeOne(this);
	if (f_ref_count <= 0) {
		f_ref_count = 0;
		delete f_dictionary;
		f_dictionary = 0;
	}
}

//-----------------------------------------------------------------------------
