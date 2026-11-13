/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDateTime>
#include <QString>

namespace stream_notify {

enum class EndAction { Keep, Delete, Edit };

struct Config {
    QString token;
    QString chat;
    QString text;
    QString button;
    QString url;
    int delay = 5;
    EndAction endAction = EndAction::Edit;
    QString endText;
    QString imagePath;
    bool captureScene = false;
};

struct TwitchSession {
    QString access;
    QString refresh;
    QString userId;
    QString login;
    QDateTime expires;
};

Config loadConfig();
void saveConfig(const Config &config);
TwitchSession loadTwitchSession();
void saveTwitchSession(const TwitchSession &session);

} // namespace stream_notify
