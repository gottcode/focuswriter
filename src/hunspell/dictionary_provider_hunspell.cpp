/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_provider_hunspell.h"

#include "dictionary_hunspell.h"
#include "../dictionary_manager.h"

#include <QDir>
#include <QListIterator>
#include <QRegExp>
#include <QStringList>

//-----------------------------------------------------------------------------

DictionaryProviderHunspell::DictionaryProviderHunspell()
{
	QStringList dictdirs;
	dictdirs.append(DictionaryManager::path());
#if !defined(Q_OS_MAC) && defined(Q_OS_UNIX)
	QStringList xdg = QString(qgetenv("XDG_DATA_DIRS")).split(QChar(':'), QString::SkipEmptyParts);
	if (xdg.isEmpty()) {
		xdg.append("/usr/local/share");
		xdg.append("/usr/share");
	}
	QStringList subdirs = QStringList() << "/hunspell" << "/myspell/dicts" << "/myspell";
	foreach (const QString& subdir, subdirs) {
		foreach (const QString& dir, xdg) {
			QString path = dir + subdir;
			if (!dictdirs.contains(path)) {
				dictdirs.append(path);
			}
		}
	}
#endif
	QDir::setSearchPaths("dict", dictdirs);
}

//-----------------------------------------------------------------------------

QStringList DictionaryProviderHunspell::availableDictionaries() const
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

AbstractDictionary* DictionaryProviderHunspell::requestDictionary(const QString& language) const
{
	return new DictionaryHunspell(language);
}

//-----------------------------------------------------------------------------
