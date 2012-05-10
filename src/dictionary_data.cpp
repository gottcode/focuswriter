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

#include "dictionary_data.h"

#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextCodec>

#include <hunspell.hxx>

//-----------------------------------------------------------------------------

DictionaryData::DictionaryData(const QString& language) :
	m_dictionary(0),
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
}

//-----------------------------------------------------------------------------

DictionaryData::~DictionaryData()
{
	delete m_dictionary;
}

//-----------------------------------------------------------------------------

void DictionaryData::addToSession(const QStringList& words)
{
	if (m_dictionary) {
		foreach (const QString& word, words) {
			m_dictionary->add(m_codec->fromUnicode(word).constData());
		}
	}
}

//-----------------------------------------------------------------------------

void DictionaryData::removeFromSession(const QStringList& words)
{
	if (m_dictionary) {
		foreach (const QString& word, words) {
			m_dictionary->remove(m_codec->fromUnicode(word).constData());
		}
	}
}

//-----------------------------------------------------------------------------
