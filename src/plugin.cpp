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
#include "twitch-client.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QWidget>

#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("stream-notify", "en-US")

namespace {

using namespace stream_notify;

QString text(const char *key)
{
    return QString::fromUtf8(obs_module_text(key));
}

class Plugin {
public:
    Plugin() : twitch(network)
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

    void screenshotTaken()
    {
        if (!awaitingScreenshot) return;
        awaitingScreenshot = false;
        char *path = obs_frontend_get_last_screenshot();
        const QString file = QString::fromUtf8(path);
        bfree(path);
        QByteArray image = readFile(file);
        if (!image.isEmpty()) QFile::remove(file);
        else image = readFile(config.imagePath);
        send(image);
    }

    SettingsActions settingsActions()
    {
        return {[this](const Config &draft) { sendTest(draft); }, twitch.login(),
                [this](std::function<void(const QString &)> setStatus) {
                    twitch.connectAccount([setStatus](const QString &status) {
                        blog(LOG_INFO, "[stream-notify] Twitch: %s", status.toUtf8().constData());
                        setStatus(status);
                    });
                }};
    }

    void sendTest(const Config &draft)
    {
        auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
        if (draft.token.isEmpty() || draft.chat.isEmpty()) {
            QMessageBox::warning(mainWindow, text("TestPost.Title"), text("TestPost.MissingSettings"));
            return;
        }
        twitch.fetchChannel([this, draft](const ChannelInfo &info) { sendTestPost(draft, info); });
    }

    void sendTestPost(const Config &draft, const ChannelInfo &info)
    {
        auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
        auto test = std::make_shared<TelegramNotifier>(network, draft.token, draft.chat);
        const auto testValues = postValues(draft, info);
        auto warn = [mainWindow](const QString &message) {
            QMessageBox::warning(mainWindow, text("TestPost.Title"), message);
        };
        test->verify([test, draft, testValues, mainWindow, warn](qint64, const QString &error) {
            if (!error.isEmpty()) {
                warn(text("TestPost.InvalidToken").arg(error));
                return;
            }
            const Post post{renderPostText(draft.text, testValues), draft.button, testValues.value("url"),
                            readFile(draft.imagePath)};
            test->publish(post, [test, mainWindow, warn](qint64, const QString &publishError) {
                if (publishError.isEmpty())
                    QMessageBox::information(mainWindow, text("TestPost.Title"), text("TestPost.Sent"));
                else warn(text("TestPost.Failed").arg(publishError));
            });
        });
    }

    void streamingStopped()
    {
        ++session;
        awaitingScreenshot = false;
        timer.stop();
        if (messageId > 0) finishPost(notifier, messageId);
        messageId = 0;
    }

private:
    Config config;
    QTimer timer;
    int session = 0;
    qint64 messageId = 0;
    bool awaitingScreenshot = false;
    QHash<QString, QString> values;
    std::shared_ptr<Notifier> notifier;
    QNetworkAccessManager network;
    TwitchClient twitch;

    void publish()
    {
        if (!obs_frontend_streaming_active()) return;
        if (config.token.isEmpty() || config.chat.isEmpty()) {
            blog(LOG_WARNING, "[stream-notify] Telegram bot token or channel is not set");
            return;
        }
        twitch.fetchChannel([this, expected = session](const ChannelInfo &info) {
            if (expected != session || !obs_frontend_streaming_active()) return;
            values = postValues(config, info);
            prepareImage();
        });
    }

    QHash<QString, QString> postValues(const Config &source, const ChannelInfo &info) const
    {
        return {{"title", info.title},
                {"category", info.category},
                {"url", resolveStreamUrl(source.url, twitch.login())}};
    }

    void prepareImage()
    {
        if (!config.captureScene) {
            send(readFile(config.imagePath));
            return;
        }
        awaitingScreenshot = true;
        obs_frontend_take_screenshot();
        QTimer::singleShot(5000, &timer, [this, expected = session] {
            if (!awaitingScreenshot || expected != session) return;
            awaitingScreenshot = false;
            blog(LOG_WARNING, "[stream-notify] Scene screenshot timed out");
            send(readFile(config.imagePath));
        });
    }

    static QByteArray readFile(const QString &path)
    {
        QFile file(path);
        if (path.isEmpty() || !file.open(QIODevice::ReadOnly)) return {};
        return file.readAll();
    }

    void send(const QByteArray &image)
    {
        const Post post{renderPostText(config.text, values), config.button, values.value("url"), image};
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
        else target->edit(id, Post{renderPostText(config.endText, values), {}, {}, {}}, done);
    }
};

std::unique_ptr<Plugin> plugin;

void frontendEvent(obs_frontend_event event, void *)
{
    if (!plugin) return;
    if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) plugin->streamingStarted();
    else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) plugin->streamingStopped();
    else if (event == OBS_FRONTEND_EVENT_SCREENSHOT_TAKEN) plugin->screenshotTaken();
    else if (event == OBS_FRONTEND_EVENT_EXIT) plugin.reset();
}

void openSettings(void *)
{
    auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
    auto config = loadConfig();
    if (!plugin) return;
    if (showSettingsDialog(mainWindow, config, plugin->settingsActions())) saveConfig(config);
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
