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

namespace stream_notify::telegram {

QUrl endpoint(const QString &token, const QString &method)
{
    return QUrl("https://api.telegram.org/bot" + token + "/" + method);
}

QString limitText(const QString &text, int limit)
{
    if (text.size() <= limit) return text;
    QString result = text.left(limit - 1);
    if (!result.isEmpty() && result.back().isHighSurrogate()) result.chop(1);
    return result + QChar(0x2026);
}

QUrlQuery messageFields(const QString &chat, const Post &post, const QString &textField, int limit)
{
    QUrlQuery form;
    form.addQueryItem("chat_id", chat);
    form.addQueryItem(textField, limitText(post.text, limit));
    const QString markup = replyMarkup(post.buttonText, post.url);
    if (!markup.isEmpty()) form.addQueryItem("reply_markup", markup);
    return form;
}

namespace {

// QUrlQuery leaves "+" unencoded, and Telegram decodes it as a space.
QByteArray encode(const QUrlQuery &form)
{
    QByteArray body;
    for (const auto &[name, value] : form.queryItems(QUrl::FullyDecoded)) {
        if (!body.isEmpty()) body += '&';
        body += QUrl::toPercentEncoding(name) + '=' + QUrl::toPercentEncoding(value);
    }
    return body;
}

} // namespace

QByteArray sendMessageBody(const QString &chat, const Post &post)
{
    return encode(messageFields(chat, post, "text", textLimit));
}

QByteArray editMessageBody(const QString &chat, qint64 messageId, const Post &post)
{
    QUrlQuery form = messageFields(chat, post, "text", textLimit);
    form.addQueryItem("message_id", QString::number(messageId));
    return encode(form);
}

QByteArray editCaptionBody(const QString &chat, qint64 messageId, const Post &post)
{
    QUrlQuery form = messageFields(chat, post, "caption", captionLimit);
    form.addQueryItem("message_id", QString::number(messageId));
    return encode(form);
}

QByteArray deleteMessageBody(const QString &chat, qint64 messageId)
{
    QUrlQuery form;
    form.addQueryItem("chat_id", chat);
    form.addQueryItem("message_id", QString::number(messageId));
    return encode(form);
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
