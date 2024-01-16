/*
	SPDX-FileCopyrightText: 2013-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "dictionary_provider_voikko.h"

#include "abstract_dictionary.h"
#include "dictionary_manager.h"
#include "smart_quotes.h"
#include "word_ref.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>

//-----------------------------------------------------------------------------

namespace
{
	const int VOIKKO_SPELL_OK = 1;
	const int VOIKKO_OPT_IGNORE_NUMBERS = 1;
	const int VOIKKO_OPT_IGNORE_UPPERCASE = 3;

	struct VoikkoHandle;

	typedef struct VoikkoHandle* (*VoikkoInitFunction)(const char** error, const char* langcode, const char* path);
	VoikkoInitFunction voikkoInit = nullptr;

	typedef void (*VoikkoTerminateFunction)(struct VoikkoHandle* handle);
	VoikkoTerminateFunction voikkoTerminate = nullptr;

	typedef int (*VoikkoSetBooleanOptionFunction)(struct VoikkoHandle * handle, int option, int value);
	VoikkoSetBooleanOptionFunction voikkoSetBooleanOption = nullptr;

	typedef int (*VoikkoSpellCstrFunction)(struct VoikkoHandle* handle, const char* word);
	VoikkoSpellCstrFunction voikkoSpellCstr = nullptr;

	typedef char** (*VoikkoSuggestCstrFunction)(struct VoikkoHandle* handle, const char* word);
	VoikkoSuggestCstrFunction voikkoSuggestCstr = nullptr;

	typedef void (*VoikkoFreeCstrArrayFunction)(char** cstrArray);
	VoikkoFreeCstrArrayFunction voikkoFreeCstrArray = nullptr;

	typedef char** (*VoikkoListSupportedSpellingLanguagesFunction)(const char * path);
	VoikkoListSupportedSpellingLanguagesFunction voikkoListSupportedSpellingLanguages = nullptr;

	bool f_voikko_loaded = false;
	QList<VoikkoHandle*> f_handles;
	QByteArray f_voikko_path;
}

//-----------------------------------------------------------------------------

static bool f_ignore_numbers = false;
static bool f_ignore_uppercase = true;

//-----------------------------------------------------------------------------

namespace
{

class DictionaryVoikko : public AbstractDictionary
{
public:
	explicit DictionaryVoikko(const QString& language);
	~DictionaryVoikko();

	bool isValid() const override
	{
		return m_handle;
	}

	WordRef check(const QString& string, int start_at) const override;
	QStringList suggestions(const QString& word) const override;

	void addToPersonal(const QString& word) override;
	void addToSession(const QStringList& words) override;
	void removeFromSession(const QStringList& words) override;

private:
	VoikkoHandle* m_handle;
};

//-----------------------------------------------------------------------------

DictionaryVoikko::DictionaryVoikko(const QString& language)
	: m_handle(nullptr)
{
	const char* voikko_error;
	m_handle = voikkoInit(&voikko_error, language.toUtf8().constData(), f_voikko_path.constData());
	if (voikko_error) {
		qWarning("DictionaryVoikko(%s): %s", qPrintable(language), voikko_error);
	} else if (m_handle) {
		voikkoSetBooleanOption(m_handle, VOIKKO_OPT_IGNORE_NUMBERS, f_ignore_numbers);
		voikkoSetBooleanOption(m_handle, VOIKKO_OPT_IGNORE_UPPERCASE, f_ignore_uppercase);
		f_handles.append(m_handle);
	}
}

//-----------------------------------------------------------------------------

DictionaryVoikko::~DictionaryVoikko()
{
	if (m_handle) {
		f_handles.removeAll(m_handle);
		voikkoTerminate(m_handle);
	}
}

//-----------------------------------------------------------------------------

WordRef DictionaryVoikko::check(const QString& string, int start_at) const
{
	int index = -1;
	int length = 0;
	int chars = 1;
	bool is_word = false;

	const int count = string.length() - 1;
	for (int i = start_at; i <= count; ++i) {
		const QChar c = string.at(i);
		if (c.isLetterOrNumber() || c.category() == QChar::Punctuation_Dash) {
			if (index == -1) {
				index = i;
				chars = 1;
				length = 0;
			}
			length += chars;
			chars = 1;
		} else if (c != '\'' && c != u'’') {
			if (index != -1) {
				is_word = true;
			}
		} else {
			chars++;
		}

		if (is_word || (i == count && index != -1)) {
			if (voikkoSpellCstr(m_handle, string.mid(index, length).toUtf8().constData()) != VOIKKO_SPELL_OK) {
				return WordRef(index, length);
			}
			index = -1;
			is_word = false;
		}
	}

	return WordRef();
}

//-----------------------------------------------------------------------------

QStringList DictionaryVoikko::suggestions(const QString& word) const
{
	QStringList result;
	char** suggestions = voikkoSuggestCstr(m_handle, word.toUtf8().constData());
	if (suggestions) {
		for (size_t i = 0; suggestions[i]; ++i) {
			QString word = QString::fromUtf8(suggestions[i]);
			if (SmartQuotes::isEnabled()) {
				SmartQuotes::replace(word);
			}
			result.append(word);
		}
		voikkoFreeCstrArray(suggestions);
	}
	return result;
}

//-----------------------------------------------------------------------------

void DictionaryVoikko::addToPersonal(const QString& word)
{
	DictionaryManager::instance().add(word);
}

//-----------------------------------------------------------------------------

void DictionaryVoikko::addToSession(const QStringList& words)
{
	Q_UNUSED(words);
	// No personal word list support in voikko?
}

//-----------------------------------------------------------------------------

void DictionaryVoikko::removeFromSession(const QStringList& words)
{
	Q_UNUSED(words);
	// No personal word list support in voikko?
}

}

//-----------------------------------------------------------------------------

DictionaryProviderVoikko::DictionaryProviderVoikko()
{
	if (f_voikko_loaded) {
		return;
	}

	QString lib = "libvoikko";
#ifdef Q_OS_WIN
	const QStringList dictdirs = QDir::searchPaths("dict");
	for (const QString& dictdir : dictdirs) {
		lib = dictdir + "/libvoikko-1.dll";
		if (QLibrary(lib).load()) {
			f_voikko_path = QFile::encodeName(QDir::toNativeSeparators(QFileInfo(lib).path()));
			break;
		}
	}
#endif
	QLibrary voikko_lib(lib);
	if (!voikko_lib.load()) {
		return;
	}

	voikkoInit = (VoikkoInitFunction) voikko_lib.resolve("voikkoInit");
	voikkoTerminate = (VoikkoTerminateFunction) voikko_lib.resolve("voikkoTerminate");
	voikkoSetBooleanOption = (VoikkoSetBooleanOptionFunction) voikko_lib.resolve("voikkoSetBooleanOption");
	voikkoSpellCstr = (VoikkoSpellCstrFunction) voikko_lib.resolve("voikkoSpellCstr");
	voikkoSuggestCstr = (VoikkoSuggestCstrFunction) voikko_lib.resolve("voikkoSuggestCstr");
	voikkoFreeCstrArray = (VoikkoFreeCstrArrayFunction) voikko_lib.resolve("voikkoFreeCstrArray");
	voikkoListSupportedSpellingLanguages = (VoikkoListSupportedSpellingLanguagesFunction) voikko_lib.resolve("voikkoListSupportedSpellingLanguages");
	f_voikko_loaded = voikkoInit
			&& voikkoTerminate
			&& voikkoSetBooleanOption
			&& voikkoSpellCstr
			&& voikkoSuggestCstr
			&& voikkoFreeCstrArray
			&& voikkoListSupportedSpellingLanguages;
	if (!f_voikko_loaded) {
		voikkoInit = nullptr;
		voikkoTerminate = nullptr;
		voikkoSetBooleanOption = nullptr;
		voikkoSpellCstr = nullptr;
		voikkoSuggestCstr = nullptr;
		voikkoFreeCstrArray = nullptr;
		voikkoListSupportedSpellingLanguages = nullptr;
	}
}

//-----------------------------------------------------------------------------

bool DictionaryProviderVoikko::isValid() const
{
	return f_voikko_loaded;
}

//-----------------------------------------------------------------------------

QStringList DictionaryProviderVoikko::availableDictionaries() const
{
	if (!f_voikko_loaded) {
		return QStringList();
	}

	QStringList result;
	char** languages = voikkoListSupportedSpellingLanguages(f_voikko_path.constData());
	if (languages) {
		for (size_t i = 0; languages[i]; ++i) {
			result.append(QString::fromUtf8(languages[i]));
		}
		voikkoFreeCstrArray(languages);
	}
	return result;
}

//-----------------------------------------------------------------------------

AbstractDictionary* DictionaryProviderVoikko::requestDictionary(const QString& language) const
{
	if (!f_voikko_loaded) {
		return nullptr;
	}

	return new DictionaryVoikko(language);
}

//-----------------------------------------------------------------------------

void DictionaryProviderVoikko::setIgnoreNumbers(bool ignore)
{
	f_ignore_numbers = ignore;
	for (VoikkoHandle* handle : std::as_const(f_handles)) {
		voikkoSetBooleanOption(handle, VOIKKO_OPT_IGNORE_NUMBERS, ignore);
	}
}

//-----------------------------------------------------------------------------

void DictionaryProviderVoikko::setIgnoreUppercase(bool ignore)
{
	f_ignore_uppercase = ignore;
	for (VoikkoHandle* handle : std::as_const(f_handles)) {
		voikkoSetBooleanOption(handle, VOIKKO_OPT_IGNORE_UPPERCASE, ignore);
	}
}

//-----------------------------------------------------------------------------
