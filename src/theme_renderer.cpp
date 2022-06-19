/*
	SPDX-FileCopyrightText: 2014-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "theme_renderer.h"

//-----------------------------------------------------------------------------

ThemeRenderer::ThemeRenderer(QObject* parent)
	: QThread(parent)
{
}

//-----------------------------------------------------------------------------

void ThemeRenderer::create(const Theme& theme, const QSize& background, const int margin, const qreal pixelratio)
{
	// Check if already rendered
	const CacheFile file{ theme, background, QRect(), QImage(), margin, pixelratio };
	if (!isRunning()) {
		const int index = m_cache.indexOf(file);
		if (index != -1) {
			m_cache.move(index, 0);
			Q_EMIT rendered(m_cache.constFirst().image, m_cache.constFirst().foreground, file.theme);
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
		while (m_cache.count() > 10) {
			m_cache.removeLast();
		}
		Q_EMIT rendered(file.image, file.foreground, file.theme);

		// Check if done
		m_file_mutex.lock();
	} while (!m_files.isEmpty());
	m_file_mutex.unlock();
}

//-----------------------------------------------------------------------------
