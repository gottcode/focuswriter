/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef IMAGE_BUTTON_H
#define IMAGE_BUTTON_H

#include <QPushButton>

class ImageButton : public QPushButton
{
	Q_OBJECT

public:
	ImageButton(QWidget* parent = 0);

	QString image() const;
	QString toString() const;

signals:
	void changed(const QString& path);

public slots:
	void setImage(const QString& image, const QString& path);
	void unsetImage();

private slots:
	void onClicked();

private:
	QString m_image;
	QString m_path;
};

inline QString ImageButton::image() const {
	return m_image;
}

inline QString ImageButton::toString() const {
	return m_path;
}

#endif
