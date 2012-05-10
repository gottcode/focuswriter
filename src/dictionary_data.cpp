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

//-----------------------------------------------------------------------------

DictionaryData::DictionaryData(EnchantBroker* broker, const QString& language) :
	m_broker(broker),
	m_dictionary(0)
{
	// Create dictionary
	m_dictionary = enchant_broker_request_dict(m_broker, language.toUtf8().constData());
	if (!m_dictionary) {
		qWarning("enchant broker error: %s", enchant_broker_get_error(m_broker));
		return;
	}
}

//-----------------------------------------------------------------------------

DictionaryData::~DictionaryData()
{
	if (m_dictionary) {
		enchant_broker_free_dict(m_broker, m_dictionary);
	}
}

//-----------------------------------------------------------------------------

void DictionaryData::addToSession(const QStringList& words)
{
	if (m_dictionary) {
		foreach (const QString& word, words) {
			QByteArray word_utf8 = word.toUtf8();
			enchant_dict_add_to_session(m_dictionary, word_utf8.constData(), word_utf8.length());
		}
	}
}

//-----------------------------------------------------------------------------

void DictionaryData::removeFromSession(const QStringList& words)
{
	if (m_dictionary) {
		foreach (const QString& word, words) {
			QByteArray word_utf8 = word.toUtf8();
			enchant_dict_remove_from_session(m_dictionary, word_utf8.constData(), word_utf8.length());
		}
	}
}

//-----------------------------------------------------------------------------
