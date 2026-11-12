/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "config.hpp"
#include "message-format.hpp"
#include "settings-dialog.hpp"
#include "telegram-notifier.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QWidget>

#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("stream-notify", "en-US")

namespace {

using namespace stream_notify;

class Plugin {
public:
    Plugin()
    {
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &timer, [this] { publish(); });
    }

    void streamingStarted()
    {
        config = loadConfig();
        ++session;
        timer.start(config.delay * 1000);
    }

    void streamingStopped()
    {
        ++session;
        timer.stop();
        if (messageId > 0) finishPost(notifier, messageId);
        messageId = 0;
    }

private:
    Config config;
    QTimer timer;
    int session = 0;
    qint64 messageId = 0;
    std::shared_ptr<Notifier> notifier;
    QNetworkAccessManager network;

    void publish()
    {
        if (!obs_frontend_streaming_active()) return;
        if (config.token.isEmpty() || config.chat.isEmpty()) {
            blog(LOG_WARNING, "[stream-notify] Telegram bot token or channel is not set");
            return;
        }
        const Post post{renderPostText(config.text, {{"url", config.url}}), config.button, config.url};
        notifier = std::make_shared<TelegramNotifier>(network, config.token, config.chat);
        notifier->publish(post, [this, sentSession = session, sent = notifier](qint64 id, const QString &error) {
            if (!error.isEmpty()) {
                blog(LOG_WARNING, "[stream-notify] Telegram post failed: %s", error.toUtf8().constData());
                return;
            }
            blog(LOG_INFO, "[stream-notify] Telegram post %lld sent", id);
            if (sentSession == session) messageId = id;
            else finishPost(sent, id);
        });
    }

    void finishPost(const std::shared_ptr<Notifier> &target, qint64 id)
    {
        if (!target || config.endAction == EndAction::Keep) return;
        auto done = [](qint64, const QString &error) {
            if (!error.isEmpty())
                blog(LOG_WARNING, "[stream-notify] Telegram end update failed: %s", error.toUtf8().constData());
        };
        if (config.endAction == EndAction::Delete) target->remove(id, done);
        else target->edit(id, Post{renderPostText(config.endText, {{"url", config.url}}), {}, {}}, done);
    }
};

std::unique_ptr<Plugin> plugin;

void frontendEvent(obs_frontend_event event, void *)
{
    if (!plugin) return;
    if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) plugin->streamingStarted();
    else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) plugin->streamingStopped();
    else if (event == OBS_FRONTEND_EVENT_EXIT) plugin.reset();
}

void openSettings(void *)
{
    auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
    auto config = loadConfig();
    if (showSettingsDialog(mainWindow, config)) saveConfig(config);
}

// OBS does not ship a Qt TLS backend, so the plugin bundles one in data/tls.
void addBundledQtPlugins()
{
    char *backend = obs_module_file("tls/qschannelbackend.dll");
    if (!backend) return;
    QCoreApplication::addLibraryPath(QFileInfo(QString::fromUtf8(backend)).dir().absoluteFilePath(".."));
    bfree(backend);
}

} // namespace

const char *obs_module_description(void)
{
    return obs_module_text("Module.Description");
}

bool obs_module_load(void)
{
    addBundledQtPlugins();
    plugin = std::make_unique<Plugin>();
    obs_frontend_add_event_callback(frontendEvent, nullptr);
    obs_frontend_add_tools_menu_item(obs_module_text("Menu.Open"), openSettings, nullptr);
    blog(LOG_INFO, "[stream-notify] loaded");
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(frontendEvent, nullptr);
    plugin.reset();
}
