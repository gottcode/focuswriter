/*
	SPDX-FileCopyrightText: 2010-2013 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_LOCALE_DIALOG_H
#define FOCUSWRITER_LOCALE_DIALOG_H

#include <QDialog>
class QComboBox;

/**
 * Dialog to set application language.
 *
 * This class handles setting the application language when the application is
 * launched, as well as allowing the user to choose a different language for
 * future launches.
 */
class LocaleDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * Construct a dialog to choose application language.
	 *
	 * @param parent the parent widget of the dialog
	 */
	explicit LocaleDialog(QWidget* parent = nullptr);

	/**
	 * Load the stored language into the application; defaults to system language.
	 *
	 * @param appname application name to prepend to translation filenames
	 * @param datadir location to search for translations
	 */
	static void loadTranslator(const QString& appname, const QString& datadir);

	/**
	 * Fetch native language name for QLocale name.
	 *
	 * @param language QLocale name to look up
	 * @return translated language name
	 */
	static QString languageName(const QString& language);

public Q_SLOTS:
	/** Override parent function to store application language. */
	void accept() override;

private:
	/**
	 * Fetch list of application translations.
	 *
	 * @return list of QLocale names
	 */
	static QStringList findTranslations();

private:
	QComboBox* m_translations; /**< list of found translations */

	static QString m_current; /**< stored application language */
	static QString m_path; /**< location of translations; found in loadTranslator() */
	static QString m_appname; /**< application name passed to loadTranslator() */
};

#endif // FOCUSWRITER_LOCALE_DIALOG_H
