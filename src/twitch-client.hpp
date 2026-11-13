/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "config.hpp"

#include <QObject>
#include <QString>

#include <functional>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
class QUrl;
class QUrlQuery;

namespace stream_notify {

struct ChannelInfo {
    QString title;
    QString category;
};

class TwitchClient {
public:
    using StatusCallback = std::function<void(const QString &status)>;
    using ChannelCallback = std::function<void(const ChannelInfo &info)>;

    explicit TwitchClient(QNetworkAccessManager &network);

    QString login() const { return session.login; }
    void connectAccount(StatusCallback status);
    void fetchChannel(ChannelCallback done, bool allowRefresh = true);

private:
    using JsonCallback = std::function<void(const QJsonObject &json, const QString &error)>;

    QNetworkAccessManager &network;
    TwitchSession session;
    QObject context;
    QString deviceCode;
    int pollSeconds = 5;

    void schedulePoll(StatusCallback status);
    void validate(StatusCallback status);
    void refresh(std::function<void()> done);
    bool storeTokens(const QJsonObject &json);
    QNetworkReply *postForm(const QUrl &url, const QUrlQuery &form);
    static void readJson(QNetworkReply *reply, JsonCallback done);
};

} // namespace stream_notify
