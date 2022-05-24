// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTZIPREADER_H
#define QTZIPREADER_H

#include <QtGlobal>

#include <QDateTime>
#include <QFile>
#include <QString>

class QtZipReaderPrivate;

class QtZipReader
{
public:
    explicit QtZipReader(const QString &fileName, QIODevice::OpenMode mode = QIODevice::ReadOnly );

    explicit QtZipReader(QIODevice *device);
    ~QtZipReader();

    QIODevice* device() const;

    bool isReadable() const;
    bool exists() const;

    struct FileInfo
    {
        FileInfo() noexcept
            : isDir(false), isFile(false), isSymLink(false), crc(0), size(0)
        {}

        bool isValid() const noexcept { return isDir || isFile || isSymLink; }

        QString filePath;
        uint isDir : 1;
        uint isFile : 1;
        uint isSymLink : 1;
        QFile::Permissions permissions;
        uint crc;
        qint64 size;
        QDateTime lastModified;
    };

    QList<FileInfo> fileInfoList() const;
    QStringList fileList() const;
    int count() const;

    FileInfo entryInfoAt(int index) const;
    QByteArray fileData(const QString &fileName) const;
    bool extractAll(const QString &destinationDir) const;

    enum Status {
        NoError,
        FileReadError,
        FileOpenError,
        FilePermissionsError,
        FileError
    };

    Status status() const;

    void close();

    static bool canRead(QIODevice* device);

private:
    QtZipReaderPrivate *d;
    Q_DISABLE_COPY_MOVE(QtZipReader)
};
Q_DECLARE_TYPEINFO(QtZipReader::FileInfo, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QtZipReader::Status, Q_PRIMITIVE_TYPE);

#endif // QTZIPREADER_H
