/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "thumbnail_model.h"

#include "thumbnail_loader.h"

#include <QCryptographicHash>
#include <QImageReader>
#include <QPainter>

/*****************************************************************************/

namespace {
	QString preview_path;

	void fetchPreviewPath() {
		QDir dir = QDir::home();
#if defined(Q_OS_MAC)
		preview_path = QDir::homePath() + "/Library/Application Support/GottCode/Previews/";
#elif defined(Q_OS_UNIX)
		preview_path = getenv("$XDG_DATA_HOME");
		if (preview_path.isEmpty()) {
			preview_path = QDir::homePath() + "/.local/share/";
		}
		preview_path += "/gottcode/previews/";
#elif defined(Q_OS_WIN32)
		preview_path = QDir::homePath() + "/Application Data/GottCode/Previews/";
#endif
		dir.mkpath(preview_path);
	}

	QString previewFileName(const QString& path) {
		QByteArray hash = QCryptographicHash::hash(QFileInfo(path).canonicalFilePath().toUtf8(), QCryptographicHash::Sha1);
		return QString(preview_path + hash.toHex() + ".png");
	}
}

/*****************************************************************************/

ThumbnailModel::ThumbnailModel(QObject* parent)
: QDirModel(parent) {
	fetchPreviewPath();

	setFilter(QDir::Files);
	QStringList filters;
	foreach (QByteArray type, QImageReader::supportedImageFormats()) {
		filters.append("*." + type);
	}
	setNameFilters(filters);

	m_loading = ThumbnailLoader::loadingIcon();

	m_loader = new ThumbnailLoader;
	connect(m_loader, SIGNAL(generated(const QString&)), this, SLOT(generated(const QString&)));
}

/*****************************************************************************/

ThumbnailModel::~ThumbnailModel() {
	m_loader->stop();
	m_loader->wait();
	delete m_loader;
}

/*****************************************************************************/

void ThumbnailModel::clear() {
	m_loader->clear();
}

/*****************************************************************************/

QVariant ThumbnailModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DisplayRole || isDir(index)) {
		return QVariant();
	} else if (role != Qt::DecorationRole) {
		return QDirModel::data(index, role);
	}

	QString file = filePath(index);
	QString preview = previewFileName(file);
	if (QFileInfo(preview).exists()) {
		return QPixmap(preview);
	} else {
		m_loader->add(file, preview);
		return m_loading;
	}
}

/*****************************************************************************/

void ThumbnailModel::generated(const QString& file) {
	QModelIndex i = index(file);
	if (i.isValid()) {
		emit dataChanged(i, i);
	}
}

/*****************************************************************************/
