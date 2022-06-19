/*
	SPDX-FileCopyrightText: 2010-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_SESSION_H
#define FOCUSWRITER_SESSION_H

#include <QCoreApplication>
#include <QStringList>
class QSettings;

class Session
{
	Q_DECLARE_TR_FUNCTIONS(Session)

public:
	explicit Session(const QString& id);
	~Session();

	int active() const;
	QSettings* data() const;
	QStringList files() const;
	QString id() const;
	QString name() const;
	QStringList positions() const;
	QString theme() const;
	bool themeDefault() const;

	void setName(const QString& name);
	void setTheme(const QString& theme, bool is_default);

	static QString createId();
	static bool exists(const QString& name);
	static QString path();
	static QString pathFromId(const QString& id);
	static void setPath(const QString& path);

private:
	QSettings* m_data;
	QString m_id;
	QString m_name;
	bool m_default;
	static QString m_path;
};

#endif // FOCUSWRITER_SESSION_H
