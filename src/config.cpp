/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.hpp"
#include "secrets.hpp"

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

obs_data_t *openConfig()
{
    obs_data_t *data = obs_data_create_from_json_file_safe(configPath().constData(), "bak");
    return data ? data : obs_data_create();
}

void writeConfig(obs_data_t *data)
{
    writeConfig(data);
}

TwitchSession loadTwitchSession()
{
    obs_data_t *data = openConfig();
    TwitchSession session;
    session.access = unprotectSecret(readString(data, "twitch_access"));
    session.refresh = unprotectSecret(readString(data, "twitch_refresh"));
    session.userId = readString(data, "twitch_user_id");
    session.login = readString(data, "twitch_login");
    session.expires = QDateTime::fromString(readString(data, "twitch_expires"), Qt::ISODate);
    obs_data_release(data);
    return session;
}

void saveTwitchSession(const TwitchSession &session)
{
    obs_data_t *data = openConfig();
    setString(data, "twitch_access", protectSecret(session.access));
    setString(data, "twitch_refresh", protectSecret(session.refresh));
    setString(data, "twitch_user_id", session.userId);
    setString(data, "twitch_login", session.login);
    setString(data, "twitch_expires", session.expires.toString(Qt::ISODate));
    writeConfig(data);
}

void setString(obs_data_t *data, const char *name, const QString &value)
{
    obs_data_set_string(data, name, value.toUtf8().constData());
}

QString readString(obs_data_t *data, const char *name)
{
    return QString::fromUtf8(obs_data_get_string(data, name));
}

EndAction parseEndAction(const QString &value)
{
    if (value == "keep") return EndAction::Keep;
    if (value == "delete") return EndAction::Delete;
    return EndAction::Edit;
}

const char *endActionName(EndAction action)
{
    switch (action) {
    case EndAction::Keep:
        return "keep";
    case EndAction::Delete:
        return "delete";
    default:
        return "edit";
    }
}

} // namespace

Config loadConfig()
{
    Config config;
    obs_data_t *data = openConfig();
    obs_data_set_default_string(data, "post_text", obs_module_text("Post.DefaultText"));
    obs_data_set_default_string(data, "post_button", obs_module_text("Post.DefaultButton"));
    obs_data_set_default_int(data, "post_delay", 5);
    obs_data_set_default_string(data, "end_text", obs_module_text("End.DefaultText"));
    config.token = unprotectSecret(readString(data, "telegram_token"));
    config.chat = readString(data, "telegram_chat");
    config.text = readString(data, "post_text");
    config.button = readString(data, "post_button");
    config.url = readString(data, "post_url");
    config.delay = static_cast<int>(obs_data_get_int(data, "post_delay"));
    config.endAction = parseEndAction(readString(data, "end_action"));
    config.endText = readString(data, "end_text");
    config.imagePath = readString(data, "image_path");
    config.captureScene = obs_data_get_bool(data, "capture_scene");
    obs_data_release(data);
    return config;
}

void saveConfig(const Config &config)
{
    obs_data_t *data = openConfig();
    obs_data_set_string(data, "telegram_token", protectSecret(config.token).toUtf8().constData());
    obs_data_set_string(data, "telegram_chat", config.chat.toUtf8().constData());
    obs_data_set_string(data, "post_text", config.text.toUtf8().constData());
    obs_data_set_string(data, "post_button", config.button.toUtf8().constData());
    obs_data_set_string(data, "post_url", config.url.toUtf8().constData());
    obs_data_set_int(data, "post_delay", config.delay);
    obs_data_set_string(data, "end_action", endActionName(config.endAction));
    obs_data_set_string(data, "end_text", config.endText.toUtf8().constData());
    obs_data_set_string(data, "image_path", config.imagePath.toUtf8().constData());
    obs_data_set_bool(data, "capture_scene", config.captureScene);
    if (!obs_data_save_json_safe(data, configPath().constData(), "tmp", "bak"))
        blog(LOG_WARNING, "[stream-notify] Could not save settings");
    obs_data_release(data);
}

} // namespace stream_notify
