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
    call("sendMessage", telegram::sendMessageBody(chat, post), std::move(done));
}

void TelegramNotifier::edit(qint64 messageId, const Post &post, Callback done)
{
    call("editMessageText", telegram::editMessageBody(chat, messageId, post), std::move(done));
}

void TelegramNotifier::remove(qint64 messageId, Callback done)
{
    call("deleteMessage", telegram::deleteMessageBody(chat, messageId), std::move(done));
}

void TelegramNotifier::call(const QString &method, const QByteArray &body, Callback done)
{
    QNetworkRequest request(telegram::endpoint(token, method));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setTransferTimeout(15000);
    auto *reply = network.post(request, body);
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, token = token, done = std::move(done)] {
        reply->deleteLater();
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (json.value("ok").toBool()) {
            done(json.value("result").toObject().value("message_id").toInteger(), {});
            return;
        }
        const QString error = json.value("description").toString(reply->errorString());
        done(0, telegram::hideToken(error.isEmpty() ? QStringLiteral("unknown error") : error, token));
    });
}

} // namespace stream_notify
