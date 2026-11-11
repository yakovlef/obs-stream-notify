/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "telegram-api.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace stream_notify::telegram {

QUrl endpoint(const QString &token, const QString &method)
{
    return QUrl("https://api.telegram.org/bot" + token + "/" + method);
}

QByteArray sendMessageBody(const QString &chat, const Post &post)
{
    QUrlQuery form;
    form.addQueryItem("chat_id", chat);
    form.addQueryItem("text", post.text);
    const QString markup = replyMarkup(post.buttonText, post.url);
    if (!markup.isEmpty()) form.addQueryItem("reply_markup", markup);
    return form.toString(QUrl::FullyEncoded).toUtf8();
}

QString replyMarkup(const QString &buttonText, const QString &url)
{
    if (buttonText.isEmpty() || url.isEmpty()) return {};
    QJsonArray row;
    row.append(QJsonObject{{"text", buttonText}, {"url", url}});
    QJsonArray keyboard;
    keyboard.append(row);
    const QJsonObject root{{"inline_keyboard", keyboard}};
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QString hideToken(QString text, const QString &token)
{
    if (!token.isEmpty()) text.replace(token, "<token>");
    return text;
}

} // namespace stream_notify::telegram
