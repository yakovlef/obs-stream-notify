/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.hpp"

#include <obs-module.h>

#include <QDir>
#include <QFileInfo>

namespace stream_notify {

namespace {

QByteArray configPath()
{
    char *path = obs_module_config_path("config.json");
    const QString result = QString::fromUtf8(path);
    bfree(path);
    QDir().mkpath(QFileInfo(result).absolutePath());
    return result.toUtf8();
}

QString readString(obs_data_t *data, const char *name)
{
    return QString::fromUtf8(obs_data_get_string(data, name));
}

} // namespace

Config loadConfig()
{
    Config config;
    obs_data_t *data = obs_data_create_from_json_file_safe(configPath().constData(), "bak");
    if (!data) return config;
    config.token = readString(data, "telegram_token");
    config.chat = readString(data, "telegram_chat");
    obs_data_release(data);
    return config;
}

void saveConfig(const Config &config)
{
    obs_data_t *data = obs_data_create();
    obs_data_set_string(data, "telegram_token", config.token.toUtf8().constData());
    obs_data_set_string(data, "telegram_chat", config.chat.toUtf8().constData());
    if (!obs_data_save_json_safe(data, configPath().constData(), "tmp", "bak"))
        blog(LOG_WARNING, "[stream-notify] Could not save settings");
    obs_data_release(data);
}

} // namespace stream_notify
