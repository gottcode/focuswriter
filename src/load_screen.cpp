/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#include "load_screen.h"

#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>

//-----------------------------------------------------------------------------

LoadScreen::LoadScreen(QWidget* parent)
	: QLabel(parent)
{
	setPixmap(QString(":/load.png"));
	setAlignment(Qt::AlignCenter);
	setStyleSheet("LoadScreen { background-color: #666666; }");

	m_text = new QLabel(this);
	m_text->hide();
	m_text->setAlignment(Qt::AlignCenter);
	m_text->setStyleSheet("QLabel {color: black; background-color: #aaaaaa; border-top-left-radius: 0.25em; border-top-right-radius: 0.25em; padding: 0.25em; }");

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->addStretch();
	layout->addWidget(m_text, 0, Qt::AlignCenter);
}

//-----------------------------------------------------------------------------

void LoadScreen::startStep(const QString& step)
{
	m_text->setText(step);
	m_text->setVisible(!step.isEmpty());
	m_steps.append(step);
	show();
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QApplication::processEvents();
}

//-----------------------------------------------------------------------------

void LoadScreen::finishStep()
{
	QApplication::restoreOverrideCursor();
	m_steps.removeFirst();
	if (!m_steps.isEmpty()) {
		QString step = m_steps.first();
		m_text->setText(step);
		m_text->setVisible(!step.isEmpty());
		QApplication::processEvents();
	} else {
		m_text->clear();
		hide();
	}
}

//-----------------------------------------------------------------------------
