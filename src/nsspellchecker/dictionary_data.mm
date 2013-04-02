/***********************************************************************
 *
 * Copyright (C) 2012 Graeme Gott <graeme@gottcode.org>
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

#include <QStringList>
#include <QVector>

#import <AppKit/NSSpellChecker.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSAutoreleasePool.h>

//-----------------------------------------------------------------------------

static NSArray* convertList(const QStringList& words)
{
	QVector<NSString*> strings;
	foreach (const QString& word, words) {
		strings.append([NSString stringWithCharacters:reinterpret_cast<const unichar*>(word.unicode()) length:word.length()]);
	}

	NSArray* array = [NSArray arrayWithObjects:strings.constData() count:strings.size()];
	return array;
}

//-----------------------------------------------------------------------------

DictionaryData::DictionaryData(const QString& language)
{
	m_language = [[NSString alloc] initWithCharacters:reinterpret_cast<const unichar*>(language.unicode()) length:language.length()];

	m_tag = [NSSpellChecker uniqueSpellDocumentTag];
}

//-----------------------------------------------------------------------------

DictionaryData::~DictionaryData()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	[m_language release];

	[[NSSpellChecker sharedSpellChecker] closeSpellDocumentWithTag:m_tag];

	[pool release];
}

//-----------------------------------------------------------------------------

void DictionaryData::addToSession(const QStringList& words)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	[[NSSpellChecker sharedSpellChecker] setIgnoredWords:convertList(words) inSpellDocumentWithTag:m_tag];

	[pool release];
}

//-----------------------------------------------------------------------------

void DictionaryData::removeFromSession(const QStringList& words)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	QStringList session;
	NSArray* array = [[NSSpellChecker sharedSpellChecker] ignoredWordsInSpellDocumentWithTag:m_tag];
	if (array) {
		for (unsigned int i = 0; i < [array count]; ++i) {
			session += QString::fromUtf8([[array objectAtIndex: i] UTF8String]);
		}
		foreach (const QString& word, words) {
			session.removeAll(word);
		}
	}

	[[NSSpellChecker sharedSpellChecker] setIgnoredWords:convertList(session) inSpellDocumentWithTag:m_tag];

	[pool release];
}

//-----------------------------------------------------------------------------
