/***********************************************************************
 *
 * Copyright (C) 2014, 2015, 2016 Graeme Gott <graeme@gottcode.org>
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

#include "theme_renderer.h"

//-----------------------------------------------------------------------------

ThemeRenderer::ThemeRenderer(QObject* parent) :
	QThread(parent)
{
}

//-----------------------------------------------------------------------------

void ThemeRenderer::create(const Theme& theme, const QSize& background, const int margin, const qreal pixelratio)
{
	// Check if already rendered
	CacheFile file = { theme, background, QRect(), QImage(), margin, pixelratio };
	if (!isRunning()) {
		int index = m_cache.indexOf(file);
		if (index != -1) {
			m_cache.move(index, 0);
			emit rendered(m_cache.first().image, m_cache.first().foreground, file.theme);
			return;
		}
	}

	// Start render thread
	m_file_mutex.lock();
	m_files.append(file);
	m_file_mutex.unlock();

	start();
}

//-----------------------------------------------------------------------------

void ThemeRenderer::run()
{
	m_file_mutex.lock();
	do {
		// Fetch theme to render
		CacheFile file = m_files.takeLast();
		m_files.clear();
		m_file_mutex.unlock();

		// Render theme
		file.image = file.theme.render(file.background, file.foreground, file.margin, file.pixelratio);
		m_cache.prepend(file);
		while (m_cache.size() > 10) {
			m_cache.removeLast();
		}
		emit rendered(file.image, file.foreground, file.theme);

		// Check if done
		m_file_mutex.lock();
	} while (!m_files.isEmpty());
	m_file_mutex.unlock();
}

//-----------------------------------------------------------------------------
