/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_IMAGE_BUTTON_H
#define FOCUSWRITER_IMAGE_BUTTON_H

#include <QPushButton>

class ImageButton : public QPushButton
{
	Q_OBJECT

public:
	explicit ImageButton(QWidget* parent = nullptr);

	QString image() const;
	QString toString() const;

Q_SIGNALS:
	void changed(const QString& path);

public Q_SLOTS:
	void setImage(const QString& image, const QString& path);
	void unsetImage();

private Q_SLOTS:
	void onClicked();

private:
	QString m_image;
	QString m_path;
};

inline QString ImageButton::image() const
{
	return m_image;
}

inline QString ImageButton::toString() const
{
	return m_path;
}

#endif // FOCUSWRITER_IMAGE_BUTTON_H
