/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "telegram-notifier.hpp"
#include "telegram-api.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <utility>

namespace stream_notify {

TelegramNotifier::TelegramNotifier(QNetworkAccessManager &network, QString token, QString chat)
    : network(network), token(std::move(token)), chat(std::move(chat))
{
}

void TelegramNotifier::publish(const Post &post, Callback done)
{
    QNetworkRequest request(telegram::endpoint(token, "sendMessage"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setTransferTimeout(15000);
    auto *reply = network.post(request, telegram::sendMessageBody(chat, post));
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, token = token, done = std::move(done)] {
        reply->deleteLater();
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const qint64 id = json.value("result").toObject().value("message_id").toInteger();
        if (json.value("ok").toBool() && id > 0) {
            done(id, {});
            return;
        }
        done(0, telegram::hideToken(json.value("description").toString(reply->errorString()), token));
    });
}

} // namespace stream_notify
