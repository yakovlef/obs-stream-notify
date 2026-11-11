/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

namespace stream_notify {

struct Config {
    QString token;
    QString chat;
    QString text;
    QString button;
    QString url;
    int delay = 5;
};

Config loadConfig();
void saveConfig(const Config &config);

} // namespace stream_notify
