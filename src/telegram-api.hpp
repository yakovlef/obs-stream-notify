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

namespace stream_notify::telegram {

QUrl endpoint(const QString &token, const QString &method);
QByteArray sendMessageBody(const QString &chat, const Post &post);
QString replyMarkup(const QString &buttonText, const QString &url);
QString hideToken(QString text, const QString &token);

} // namespace stream_notify::telegram
