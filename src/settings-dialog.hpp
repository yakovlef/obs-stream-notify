/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "config.hpp"

#include <functional>

class QWidget;

namespace stream_notify {

struct SettingsActions {
    std::function<void(const Config &)> sendTest;
    QString twitchLogin;
    std::function<void(std::function<void(const QString &)> setStatus)> connectTwitch;
};

bool showSettingsDialog(QWidget *parent, Config &config, const SettingsActions &actions);

} // namespace stream_notify
