/*
	SPDX-FileCopyrightText: 2012-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QBuffer>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QStringList>
#include <QTextStream>
#include <QUrl>

#include <iostream>

struct Filter
{
	Filter(const QByteArray& name_ = QByteArray())
		: name(name_)
		, size(0)
	{
	}

	void addRange(char32_t start, char32_t end);

	struct Range
	{
		Range()
			: start(0)
			, end(0)
		{
		}

		Range(char32_t start_code, char32_t end_code)
			: start(start_code)
			, end(end_code)
		{
		}

		char32_t start;
		char32_t end;
	};

	QByteArray name;
	char32_t size;
	QList<Range> ranges;
};

typedef QList<Filter> FilterGroup;

void Filter::addRange(char32_t start, char32_t end)
{
	if (ranges.isEmpty() || (ranges.constLast().end != (start - 1))) {
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
		const QScopedPointer<QNetworkAccessManager> manager(new QNetworkAccessManager);

		static const QStringList filenames{"UnicodeData.txt", "Blocks.txt", "Scripts.txt"};
		for (const QString& filename : filenames) {
			std::cout << "Downloading " << filename.toStdString() << "... " << std::flush;
			if (QFile::exists(path + "/" + filename)) {
				std::cout << "SKIPPED" << std::endl;
				continue;
			}

			// Download file
			const QUrl url("http://www.unicode.org/Public/" + unicode_version + "/ucd/" + filename);
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

	QHash<char32_t, QByteArray> names;
	QList<FilterGroup> groups;

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
			const char32_t code = parts.at(0).toUInt(0, 16);
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
			const QStringList range = parts.at(0).trimmed().split('.', Qt::SkipEmptyParts);
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
			char32_t start, end;
			const QStringList range = parts.at(0).trimmed().split('.', Qt::SkipEmptyParts);
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
			if (scripts.isEmpty() || (scripts.constLast().name != name)) {
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

	downloadAndParse("15.1.0", QDataStream::Qt_6_2);
}
