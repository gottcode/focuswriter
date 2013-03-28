/***********************************************************************
 *
 * Copyright (C) 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_data.h"
#include "dictionary_manager.h"

#include <QStringList>

#import <AppKit/NSSpellChecker.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSAutoreleasePool.h>

//-----------------------------------------------------------------------------

QStringRef Dictionary::check(const QString& string, int start_at) const
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	NSString* nsstring = [NSString stringWithCharacters:reinterpret_cast<const unichar*>(string.unicode()) length:string.length()];

	QStringRef misspelled;

	NSRange range = [[NSSpellChecker sharedSpellChecker] checkSpellingOfString:nsstring
		startingAt:start_at
		language:(*d)->language()
		wrap:NO
		inSpellDocumentWithTag:(*d)->tag()
		wordCount:NULL];

	if (range.length > 0) {
		misspelled = QStringRef(&string, range.location, range.length);
	}

	[pool release];

	return misspelled;
}

//-----------------------------------------------------------------------------

QStringList Dictionary::suggestions(const QString& word) const
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	NSRange range;
	range.location = 0;
	range.length = word.length();

	NSString* nsstring = [NSString stringWithCharacters:reinterpret_cast<const unichar*>(word.unicode()) length:word.length()];

	NSArray* array;
	if ([[NSSpellChecker sharedSpellChecker] respondsToSelector:@selector(guessesForWordRange)]) {
		array = [[NSSpellChecker sharedSpellChecker] guessesForWordRange:range
			inString:nsstring
			language:(*d)->language()
			inSpellDocumentWithTag:(*d)->tag()];
	} else {
		array = [[NSSpellChecker sharedSpellChecker] guessesForWord:nsstring];
	}

	QStringList suggestions;
	if (array) {
		for (unsigned int i = 0; i < [array count]; ++i) {
			nsstring = [array objectAtIndex: i];
			suggestions += QString::fromUtf8([nsstring UTF8String]);
		}
	}

	[pool release];

	return suggestions;
}

//-----------------------------------------------------------------------------

void Dictionary::addWord(const QString& word)
{
	DictionaryManager::instance().add(word);
}

//-----------------------------------------------------------------------------
