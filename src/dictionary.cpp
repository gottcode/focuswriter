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

#include "dictionary.h"

#include "smart_quotes.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>
#include <QWeakPointer>

#include <hunspell.hxx>

//-----------------------------------------------------------------------------

class DictionaryPrivate
{
public:
	DictionaryPrivate(const QString& language);
	~DictionaryPrivate();

	QStringRef check(const QString& string, int start_at = 0) const;
	QStringList suggestions(const QString& word) const;

	void addPersonal();
	void removePersonal();

private:
	Hunspell* m_dictionary;
	QTextCodec* m_codec;
};

namespace
{
	QList<Dictionary*> f_instances;
	QHash<QString, QWeakPointer<DictionaryPrivate> > f_dictionaries;
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
	f_instances.append(this);
	setLanguage(QString());
}

//-----------------------------------------------------------------------------

Dictionary::Dictionary(const QString& language, QObject* parent)
	: QObject(parent)
{
	f_instances.append(this);
	setLanguage(language);
}

//-----------------------------------------------------------------------------

Dictionary::~Dictionary()
{
	f_instances.removeAll(this);
}

//-----------------------------------------------------------------------------

QStringRef Dictionary::check(const QString& string, int start_at) const
{
	return d->check(string, start_at);
}

//-----------------------------------------------------------------------------

QStringList Dictionary::suggestions(const QString& word) const
{
	return d->suggestions(word);
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

void Dictionary::setLanguage(const QString& language)
{
	QString l = !language.isEmpty() ? language : f_language;
	d = f_dictionaries[l];
	if (d.isNull()) {
		d = QSharedPointer<DictionaryPrivate>(new DictionaryPrivate(l));
		f_dictionaries[l] = d;
	}
	emit changed();
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

QString Dictionary::defaultLanguage()
{
	return f_language;
}

//-----------------------------------------------------------------------------

QStringList Dictionary::personal()
{
	return f_personal;
}

//-----------------------------------------------------------------------------

void Dictionary::setDefaultLanguage(const QString& language)
{
	if (language == f_language) {
		return;
	}

	QSharedPointer<DictionaryPrivate> previous = f_dictionaries[f_language];
	f_language = language;
	QSharedPointer<DictionaryPrivate> next = f_dictionaries[f_language];
	if (next.isNull()) {
		next = QSharedPointer<DictionaryPrivate>(new DictionaryPrivate(f_language));
		f_dictionaries[f_language] = next;
	}

	foreach (Dictionary* dictionary, f_instances) {
		if (dictionary->d == previous) {
			dictionary->d = next;
			emit dictionary->changed();
		}
	}
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

void Dictionary::setPersonal(const QStringList& words)
{
	// Check if new
	QStringList personal = SmartQuotes::revert(words);
	qSort(personal.begin(), personal.end(), compareWords);
	if (personal == f_personal) {
		return;
	}

	// Remove current personal dictionary
	QList<QWeakPointer<DictionaryPrivate> > dictionaries = f_dictionaries.values();
	foreach (QSharedPointer<DictionaryPrivate> dictionary, dictionaries) {
		if (!dictionary.isNull()) {
			dictionary->removePersonal();
		}
	}

	// Update and store personal dictionary
	f_personal = personal;
	QFile file(f_path + "/personal");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec(QTextCodec::codecForName("UTF-8"));
		foreach (const QString& word, f_personal) {
			stream << word << "\n";
		}
	}

	// Add personal dictionary
	foreach (QSharedPointer<DictionaryPrivate> dictionary, dictionaries) {
		if (!dictionary.isNull()) {
			dictionary->addPersonal();
		}
	}

	// Re-check documents
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

DictionaryPrivate::DictionaryPrivate(const QString& language)
	: m_dictionary(0),
	m_codec(0)
{
	// Find dictionary files
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
	if (language.isEmpty() || aff.isEmpty() || dic.isEmpty()) {
		return;
	}

	// Create dictionary
	m_dictionary = new Hunspell(QFile::encodeName(aff).constData(), QFile::encodeName(dic).constData());
	m_codec = QTextCodec::codecForName(m_dictionary->get_dic_encoding());
	if (!m_codec) {
		delete m_dictionary;
		m_dictionary = 0;
		return;
	}

	// Add personal dictionary
	if (f_personal.isEmpty()) {
		QFile file(f_path + "/personal");
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream stream(&file);
			stream.setCodec(QTextCodec::codecForName("UTF-8"));
			while (!stream.atEnd()) {
				f_personal.append(stream.readLine());
			}
			qSort(f_personal.begin(), f_personal.end(), compareWords);
		}
	}
	addPersonal();
}

//-----------------------------------------------------------------------------

DictionaryPrivate::~DictionaryPrivate()
{
	delete m_dictionary;
}

//-----------------------------------------------------------------------------

QStringRef DictionaryPrivate::check(const QString& string, int start_at) const
{
	if (!m_dictionary) {
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
				QString word = check.toString();
				word.replace(QChar(0x2019), QLatin1Char('\''));
				if (!m_dictionary->spell(m_codec->fromUnicode(word).constData())) {
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

QStringList DictionaryPrivate::suggestions(const QString& word) const
{
	QStringList result;
	if (!m_dictionary) {
		return result;
	}

	char** suggestions = 0;
	int count = m_dictionary->suggest(&suggestions, m_codec->fromUnicode(word).constData());
	if (suggestions != 0) {
		for (int i = 0; i < count; ++i) {
			QString word = m_codec->toUnicode(suggestions[i]);
			SmartQuotes::replace(word);
			result.append(word);
		}
		m_dictionary->free_list(&suggestions, count);
	}
	return result;
}

//-----------------------------------------------------------------------------

void DictionaryPrivate::addPersonal()
{
	if (m_dictionary) {
		foreach (const QString& word, f_personal) {
			m_dictionary->add(m_codec->fromUnicode(word).constData());
		}
	}
}

//-----------------------------------------------------------------------------

void DictionaryPrivate::removePersonal()
{
	if (m_dictionary) {
		foreach (const QString& word, f_personal) {
			m_dictionary->remove(m_codec->fromUnicode(word).constData());
		}
	}
}

//-----------------------------------------------------------------------------
