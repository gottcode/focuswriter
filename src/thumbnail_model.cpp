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

	m_loading = QPixmap(100, 100);
	m_loading.fill(QColor(0, 0, 0, 0));
	{
		QPainter painter(&m_loading);
		painter.translate(32, 32);
		painter.setRenderHint(QPainter::Antialiasing, true);

		painter.setPen(QColor(100, 100, 100));
		painter.setBrush(QColor(200, 200, 200));
		painter.drawEllipse(0, 0, 36, 36);

		painter.setBrush(Qt::white);
		painter.drawEllipse(2, 2, 32, 32);

		painter.setPen(QPen(QColor(100, 100, 100), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.drawPoint(18, 6);
		painter.drawPoint(18, 30);
		painter.drawPoint(6, 18);
		painter.drawPoint(30, 18);

		painter.setPen(QPen(QColor(0, 0, 0), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.setBrush(QColor(0, 0, 0, 0));
		painter.drawEllipse(16, 16, 4, 4);
		painter.drawLine(20, 20, 27, 24);

		painter.setPen(QPen(QColor(0, 0, 0), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.drawLine(19, 16, 22, 6);
	}

	setFilter(QDir::Files);
	QStringList filters;
	foreach (QByteArray type, QImageReader::supportedImageFormats()) {
		filters.append("*." + type);
	}
	setNameFilters(filters);

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
