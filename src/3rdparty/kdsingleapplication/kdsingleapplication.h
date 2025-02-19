/*
  This file is part of KDSingleApplication.

  SPDX-FileCopyrightText: 2019 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/
#ifndef KDSINGLEAPPLICATION_H
#define KDSINGLEAPPLICATION_H

#include <QtCore/QObject>
#include <QtCore/QFlags>

#include <memory>

#include "kdsingleapplication_lib.h"

class KDSingleApplicationPrivate;

class KDSINGLEAPPLICATION_EXPORT KDSingleApplication : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool isPrimaryInstance READ isPrimaryInstance CONSTANT)

public:
    // IncludeUsernameInSocketName - Include the username in the socket name.
    // IncludeSessionInSocketName - Include the graphical session in the socket name.
    enum class Option {
        None = 0x0,
        IncludeUsernameInSocketName = 0x1,
        IncludeSessionInSocketName = 0x2,
    };
    Q_DECLARE_FLAGS(Options, Option)

    explicit KDSingleApplication(QObject *parent = nullptr);
    explicit KDSingleApplication(const QString &name, QObject *parent = nullptr);
    explicit KDSingleApplication(const QString &name, Options options, QObject *parent = nullptr);
    ~KDSingleApplication();

    QString name() const;
    bool isPrimaryInstance() const;

public Q_SLOTS:
    // avoid default arguments and overloads, as they don't mix with connections
    bool sendMessage(const QByteArray &message);
    bool sendMessageWithTimeout(const QByteArray &message, int timeout);

Q_SIGNALS:
    void messageReceived(const QByteArray &message);

private:
    Q_DECLARE_PRIVATE(KDSingleApplication)
    std::unique_ptr<KDSingleApplicationPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KDSingleApplication::Options)

#endif // KDSINGLEAPPLICATION_H
