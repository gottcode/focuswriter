/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_manager.h"

#include "dictionary.h"
#include "smart_quotes.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

//-----------------------------------------------------------------------------

namespace
{
	bool compareWords(const QString& s1, const QString& s2)
	{
		return s1.localeAwareCompare(s2) < 0;
	}
}

QString DictionaryManager::m_path;

//-----------------------------------------------------------------------------

DictionaryManager& DictionaryManager::instance()
{
	static DictionaryManager manager;
	return manager;
}

//-----------------------------------------------------------------------------

QStringList DictionaryManager::availableDictionaries() const
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

void DictionaryManager::add(const QString& word)
{
	QStringList words = personal();
	if (words.contains(SmartQuotes::revert(word))) {
		return;
	}
	words.append(word);
	setPersonal(words);
}

//-----------------------------------------------------------------------------

Dictionary** DictionaryManager::requestDictionary(const QString& language)
{
	if (language.isEmpty()) {
		// Fetch shared default dictionary
		if (!m_default_dictionary) {
			m_default_dictionary = new Dictionary(m_default_language);
		}
		return &m_default_dictionary;
	} else {
		// Fetch specific dictionary
		if (!m_dictionaries.contains(language)) {
			m_dictionaries[language] = new Dictionary(language);
		}
		return &m_dictionaries[language];
	}
}

//-----------------------------------------------------------------------------

void DictionaryManager::setDefaultLanguage(const QString& language)
{
	if (language == m_default_language) {
		return;
	}
	m_default_language = language;

	// Change pointer for default dictionary
	Dictionary* dictionary = m_dictionaries[m_default_language];
	if (!dictionary) {
		dictionary = new Dictionary(m_default_language);
		m_dictionaries[m_default_language] = dictionary;
	}
	m_default_dictionary = dictionary;

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

void DictionaryManager::setIgnoreNumbers(bool ignore)
{
	m_ignore_numbers = ignore;
}

//-----------------------------------------------------------------------------

void DictionaryManager::setIgnoreUppercase(bool ignore)
{
	m_ignore_uppercase = ignore;
}

//-----------------------------------------------------------------------------

void DictionaryManager::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------

void DictionaryManager::setPersonal(const QStringList& words)
{
	// Check if new
	QStringList personal = SmartQuotes::revert(words);
	qSort(personal.begin(), personal.end(), compareWords);
	if (personal == m_personal) {
		return;
	}

	// Remove current personal dictionary
	foreach (Dictionary* dictionary, m_dictionaries) {
		dictionary->removePersonal();
	}

	// Update and store personal dictionary
	m_personal = personal;
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		foreach (const QString& word, m_personal) {
			stream << word << "\n";
		}
	}

	// Add personal dictionary
	foreach (Dictionary* dictionary, m_dictionaries) {
		dictionary->addPersonal();
	}

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

DictionaryManager::DictionaryManager() :
	m_ignore_numbers(false),
	m_ignore_uppercase(true)
{
	// Load personal dictionary
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		while (!stream.atEnd()) {
			m_personal.append(stream.readLine());
		}
		qSort(m_personal.begin(), m_personal.end(), compareWords);
	}
}

//-----------------------------------------------------------------------------

DictionaryManager::~DictionaryManager()
{
	foreach (Dictionary* dictionary, m_dictionaries) {
		delete dictionary;
	}
}

//-----------------------------------------------------------------------------
