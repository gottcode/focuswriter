/***********************************************************************
 *
 * Copyright (C) 2012, 2013, 2014, 2016, 2018 Graeme Gott <graeme@gottcode.org>
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

#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <QVector>

#include <iostream>

struct Filter
{
	Filter(const QByteArray& name_ = QByteArray()) :
		name(name_), size(0) { }

	void addRange(quint32 start, quint32 end);

	struct Range
	{
		Range() :
			start(0), end(0) { }

		Range(quint32 start_code, quint32 end_code) :
			start(start_code), end(end_code) { }

		quint32 start;
		quint32 end;
	};

	QByteArray name;
	quint32 size;
	QVector<Range> ranges;
};

typedef QVector<Filter> FilterGroup;

void Filter::addRange(quint32 start, quint32 end)
{
	if (ranges.isEmpty() || (ranges.last().end != (start - 1))) {
		ranges += Filter::Range(start, end);
	} else {
		ranges.last().end = end;
	}
	size += (end - start + 1);
}

QDataStream& operator<<(QDataStream& stream, const Filter::Range& range)
{
	stream << range.start << range.end;
	return stream;
}

QDataStream& operator<<(QDataStream& stream, const Filter& filter)
{
	stream << filter.name << filter.size << filter.ranges;
	return stream;
}

int downloadAndParse(const QString& unicode_version, QDataStream::Version data_version)
{
	const QString path = QString(unicode_version).remove('.').prepend("symbols");

	// Create location for data
	{
		QDir dir(path);
		dir.mkdir(dir.absolutePath());
	}

	// Download necessary Unicode data files
	{
		QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);

		const QStringList filenames{"UnicodeData.txt", "Blocks.txt", "Scripts.txt"};
		for (const QString& filename : filenames) {
			std::cout << "Downloading " << filename.toStdString() << "... " << std::flush;
			if (QFile::exists(path + "/" + filename)) {
				std::cout << "SKIPPED" << std::endl;
				continue;
			}

			// Download file
			QUrl url("http://www.unicode.org/Public/" + unicode_version + "/ucd/" + filename);
			QNetworkReply* reply = manager->get(QNetworkRequest(url));

			QEventLoop loop;
			QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
			loop.exec();

			// Write file to disk
			QFile file(path + "/" + filename);
			if (!file.open(QFile::WriteOnly | QFile::Text)) {
				std::cout << "ERROR" << std::endl;
				return 1;
			}

			file.write(reply->readAll());

			file.close();
			std::cout << "DONE" << std::endl;
		}
	}

	QHash<quint32, QByteArray> names;
	QVector<FilterGroup> groups;

	// Parse names
	{
		std::cout << "Parsing names... " << std::flush;
		QFile file(path + "/UnicodeData.txt");
		if (!file.open(QFile::ReadOnly | QFile::Text)) {
			std::cout << "ERROR" << std::endl;
			return 1;
		}

		QTextStream stream(&file);
		while (!stream.atEnd()) {
			const QStringList parts = stream.readLine().split(";");
			const quint32 code = parts.at(0).toUInt(0, 16);
			QString name = parts.at(1);
			if (name.startsWith('<')) {
				name = parts.at(10);
			}
			if (!name.isEmpty()) {
				names[code] = name.toLatin1();
			}
		}

		file.close();
		std::cout << "DONE" << std::endl;
	}

	// Parse blocks
	{
		FilterGroup blocks;

		std::cout << "Parsing blocks... " << std::flush;
		QFile file(path + "/Blocks.txt");
		if (!file.open(QFile::ReadOnly | QFile::Text)) {
			std::cout << "ERROR" << std::endl;
			return 1;
		}

		QTextStream stream(&file);
		while (!stream.atEnd()) {
			QString line = stream.readLine().trimmed();
			if (line.isEmpty() || line.startsWith('#')) {
				continue;
			}

			// Remove comment
			const int comment_start = line.indexOf('#');
			if (comment_start != -1) {
				line = line.left(comment_start);
			}
			const QStringList parts = line.split(";");

			// Find block code point range
			const QStringList range = parts.at(0).trimmed().split('.', QString::SkipEmptyParts);
			if (range.count() != 2) {
				continue;
			}

			// Find block name
			Filter block;
			block.name = parts.at(1).trimmed().toLatin1();
			block.name = block.name.replace("_", " ");

			// Set range for block
			block.addRange(range.at(0).toUInt(0, 16), range.at(1).toUInt(0, 16));
			blocks += block;
		}

		file.close();
		std::cout << "DONE" << std::endl;

		groups += blocks;
	}

	// Parse scripts
	{
		FilterGroup scripts;

		std::cout << "Parsing scripts... " << std::flush;
		QFile file(path + "/Scripts.txt");
		if (!file.open(QFile::ReadOnly | QFile::Text)) {
			std::cout << "ERROR" << std::endl;
			return 1;
		}

		QTextStream stream(&file);
		while (!stream.atEnd()) {
			QString line = stream.readLine().trimmed();
			if (line.isEmpty() || line.startsWith('#')) {
				continue;
			}

			// Remove comment
			const int comment_start = line.indexOf('#');
			if (comment_start != -1) {
				line = line.left(comment_start);
			}
			const QStringList parts = line.split(";");

			// Find script code point range
			quint32 start, end;
			QStringList range = parts.at(0).trimmed().split('.', QString::SkipEmptyParts);
			if (range.count() == 1) {
				start = end = range.at(0).toUInt(0, 16);
			} else if (range.count() == 2) {
				start = range.at(0).toUInt(0, 16);
				end = range.at(1).toUInt(0, 16);
			} else {
				continue;
			}

			// Find script name
			QByteArray name = parts.at(1).trimmed().toLatin1();
			name = name.replace("_", " ");
			if (name == "Nko") {
				name = "N'Ko";
			}
			if (scripts.isEmpty() || (scripts.last().name != name)) {
				scripts += name;
			}

			// Append range to script
			scripts.last().addRange(start, end);
		}

		file.close();
		std::cout << "DONE" << std::endl;

		groups += scripts;
	}

	// Write symbols
	{
		std::cout << "Writing symbols... " << std::flush;

		QBuffer buffer;
		if (!buffer.open(QIODevice::WriteOnly)) {
			std::cout << "ERROR" << std::endl;
			return 1;
		}

		QDataStream stream(&buffer);
		stream.setVersion(data_version);
		stream << names;
		stream << groups;
		buffer.close();

		QFile file(path + ".dat");
		if (!file.open(QFile::WriteOnly)) {
			std::cout << "ERROR" << std::endl;
			return 1;
		}
		file.write(qCompress(buffer.data(), 9));
		file.close();

		std::cout << "DONE" << std::endl;
	}

	return 0;
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);

	downloadAndParse("10.0.0", QDataStream::Qt_5_2);
}
