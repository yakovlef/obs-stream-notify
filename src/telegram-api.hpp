/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "notifier.hpp"

#include <QByteArray>
#include <QUrl>
#include <QUrlQuery>

namespace stream_notify::telegram {

constexpr int textLimit = 4096;
constexpr int captionLimit = 1024;

QUrl endpoint(const QString &token, const QString &method);
QString limitText(const QString &text, int limit);
QUrlQuery messageFields(const QString &chat, const Post &post, const QString &textField, int limit);
QByteArray sendMessageBody(const QString &chat, const Post &post);
QByteArray editMessageBody(const QString &chat, qint64 messageId, const Post &post);
QByteArray editCaptionBody(const QString &chat, qint64 messageId, const Post &post);
QByteArray deleteMessageBody(const QString &chat, qint64 messageId);
QString replyMarkup(const QString &buttonText, const QString &url);
QString hideToken(QString text, const QString &token);

} // namespace stream_notify::telegram
