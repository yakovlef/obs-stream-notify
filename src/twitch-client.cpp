/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "twitch-client.hpp"

#include <obs-module.h>

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <utility>

namespace stream_notify {

namespace {

constexpr auto clientId = "axoa5y1um81tof8nni7baus7m7wl8t";
const QUrl tokenUrl("https://id.twitch.tv/oauth2/token");

QString text(const char *key)
{
    return QString::fromUtf8(obs_module_text(key));
}

} // namespace

TwitchClient::TwitchClient(QNetworkAccessManager &network) : network(network), session(loadTwitchSession()) {}

void TwitchClient::connectAccount(StatusCallback status)
{
    status(text("Twitch.RequestingCode"));
    QUrlQuery form;
    form.addQueryItem("client_id", clientId);
    form.addQueryItem("scopes", "");
    readJson(postForm(QUrl("https://id.twitch.tv/oauth2/device"), form),
             [this, status](const QJsonObject &json, const QString &error) {
                 const QString code = json.value("device_code").toString();
                 if (code.isEmpty()) {
                     status(text("Twitch.Failed").arg(json.value("message").toString(error)));
                     return;
                 }
                 deviceCode = code;
                 pollSeconds = qMax(5, json.value("interval").toInt(5));
                 status(text("Twitch.EnterCode").arg(json.value("user_code").toString()));
                 QDesktopServices::openUrl(QUrl(json.value("verification_uri").toString()));
                 QTimer::singleShot(json.value("expires_in").toInt(1800) * 1000, &context, [this, code, status] {
                     if (deviceCode != code) return;
                     deviceCode.clear();
                     status(text("Twitch.CodeExpired"));
                 });
                 schedulePoll(status);
             });
}

void TwitchClient::schedulePoll(StatusCallback status)
{
    QTimer::singleShot(pollSeconds * 1000, &context, [this, status, code = deviceCode] {
        if (code.isEmpty() || deviceCode != code) return;
        QUrlQuery form;
        form.addQueryItem("client_id", clientId);
        form.addQueryItem("device_code", code);
        form.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");
        readJson(postForm(tokenUrl, form), [this, status](const QJsonObject &json, const QString &error) {
            if (storeTokens(json)) {
                deviceCode.clear();
                validate(status);
                return;
            }
            const QString message = json.value("message").toString(error);
            if (message == "authorization_pending") {
                schedulePoll(status);
            } else if (message == "slow_down") {
                pollSeconds += 5;
                schedulePoll(status);
            } else {
                deviceCode.clear();
                status(text("Twitch.Failed").arg(message));
            }
        });
    });
}

void TwitchClient::validate(StatusCallback status)
{
    QNetworkRequest request(QUrl("https://id.twitch.tv/oauth2/validate"));
    request.setRawHeader("Authorization", "OAuth " + session.access.toUtf8());
    request.setTransferTimeout(15000);
    readJson(network.get(request), [this, status](const QJsonObject &json, const QString &error) {
        session.userId = json.value("user_id").toString();
        session.login = json.value("login").toString();
        saveTwitchSession(session);
        if (session.login.isEmpty()) status(text("Twitch.Failed").arg(json.value("message").toString(error)));
        else status(text("Twitch.Connected").arg(session.login));
    });
}

void TwitchClient::refresh(std::function<void()> done)
{
    if (session.refresh.isEmpty()) {
        done();
        return;
    }
    QUrlQuery form;
    form.addQueryItem("client_id", clientId);
    form.addQueryItem("grant_type", "refresh_token");
    form.addQueryItem("refresh_token", session.refresh);
    readJson(postForm(tokenUrl, form), [this, done = std::move(done)](const QJsonObject &json, const QString &error) {
        if (!storeTokens(json))
            blog(LOG_WARNING, "[stream-notify] Twitch token refresh failed: %s",
                 json.value("message").toString(error).toUtf8().constData());
        saveTwitchSession(session);
        done();
    });
}

void TwitchClient::fetchChannel(ChannelCallback done, bool allowRefresh)
{
    if (session.userId.isEmpty()) {
        done({});
        return;
    }
    if (allowRefresh && session.expires.isValid() && session.expires < QDateTime::currentDateTimeUtc().addSecs(60)) {
        refresh([this, done = std::move(done)] { fetchChannel(done, false); });
        return;
    }
    QUrl url("https://api.twitch.tv/helix/channels");
    url.setQuery(QUrlQuery{{"broadcaster_id", session.userId}});
    QNetworkRequest request(url);
    request.setRawHeader("Client-Id", clientId);
    request.setRawHeader("Authorization", "Bearer " + session.access.toUtf8());
    request.setTransferTimeout(15000);
    readJson(network.get(request), [this, done, allowRefresh](const QJsonObject &json, const QString &error) {
        const QJsonArray channels = json.value("data").toArray();
        if (!channels.isEmpty()) {
            const QJsonObject channel = channels.at(0).toObject();
            done({channel.value("title").toString(), channel.value("game_name").toString()});
            return;
        }
        if (allowRefresh) {
            refresh([this, done] { fetchChannel(done, false); });
            return;
        }
        blog(LOG_WARNING, "[stream-notify] Twitch channel info failed: %s",
             json.value("message").toString(error).toUtf8().constData());
        done({});
    });
}

bool TwitchClient::storeTokens(const QJsonObject &json)
{
    const QString access = json.value("access_token").toString();
    if (access.isEmpty()) return false;
    session.access = access;
    session.refresh = json.value("refresh_token").toString();
    session.expires = QDateTime::currentDateTimeUtc().addSecs(json.value("expires_in").toInt(14400));
    return true;
}

QNetworkReply *TwitchClient::postForm(const QUrl &url, const QUrlQuery &form)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setTransferTimeout(15000);
    return network.post(request, form.toString(QUrl::FullyEncoded).toUtf8());
}

void TwitchClient::readJson(QNetworkReply *reply, JsonCallback done)
{
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, done = std::move(done)] {
        reply->deleteLater();
        done(QJsonDocument::fromJson(reply->readAll()).object(), reply->errorString());
    });
}

} // namespace stream_notify
