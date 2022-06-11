/*
	SPDX-FileCopyrightText: 2009-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_DICTIONARY_MANAGER_H
#define FOCUSWRITER_DICTIONARY_MANAGER_H

class AbstractDictionary;
class AbstractDictionaryProvider;
class DictionaryRef;

#include <QHash>
#include <QObject>
#include <QStringList>

class DictionaryManager : public QObject
{
	Q_OBJECT

public:
	static DictionaryManager& instance();

	QStringList availableDictionaries() const;
	QString availableDictionary(const QString& language) const;
	QString defaultLanguage() const;
	QStringList personal() const;

	void add(const QString& word);
	void addProviders();
	DictionaryRef requestDictionary(const QString& language = QString());
	void setDefaultLanguage(const QString& language);
	void setIgnoreNumbers(bool ignore);
	void setIgnoreUppercase(bool ignore);
	void setPersonal(const QStringList& words);

	static QString installedPath();
	static QString path();
	static void setPath(const QString& path);

Q_SIGNALS:
	void changed();

private:
	DictionaryManager();
	~DictionaryManager();

	void addProvider(AbstractDictionaryProvider* provider);
	AbstractDictionary** requestDictionaryData(const QString& language);

private:
	QList<AbstractDictionaryProvider*> m_providers;
	QHash<QString, AbstractDictionary*> m_dictionaries;
	AbstractDictionary* m_default_dictionary;

	QString m_default_language;
	QStringList m_personal;

	static QString m_path;
};

inline QString DictionaryManager::defaultLanguage() const
{
	return m_default_language;
}

inline QString DictionaryManager::path()
{
	return m_path;
}

inline QStringList DictionaryManager::personal() const
{
	return m_personal;
}

#endif // FOCUSWRITER_DICTIONARY_MANAGER_H
