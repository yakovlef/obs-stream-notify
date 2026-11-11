/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

#include <functional>

namespace stream_notify {

struct Post {
    QString text;
    QString buttonText;
    QString url;
};

class Notifier {
public:
    using Callback = std::function<void(qint64 messageId, const QString &error)>;

    virtual ~Notifier() = default;
    virtual void publish(const Post &post, Callback done) = 0;
};

} // namespace stream_notify
