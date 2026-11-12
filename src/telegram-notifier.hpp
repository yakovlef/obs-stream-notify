/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "notifier.hpp"

class QNetworkAccessManager;

namespace stream_notify {

class TelegramNotifier final : public Notifier {
public:
    TelegramNotifier(QNetworkAccessManager &network, QString token, QString chat);
    void publish(const Post &post, Callback done) override;
    void edit(qint64 messageId, const Post &post, Callback done) override;
    void remove(qint64 messageId, Callback done) override;

private:
    void call(const QString &method, const QByteArray &body, Callback done);

    QNetworkAccessManager &network;
    QString token;
    QString chat;
};

} // namespace stream_notify
