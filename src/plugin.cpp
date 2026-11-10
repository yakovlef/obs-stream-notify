/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "config.hpp"
#include "settings-dialog.hpp"

#include <QWidget>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("stream-notify", "en-US")

namespace {

void frontendEvent(obs_frontend_event event, void *)
{
    if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED)
        blog(LOG_INFO, "[stream-notify] Streaming started");
    else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED)
        blog(LOG_INFO, "[stream-notify] Streaming stopped");
}

void openSettings(void *)
{
    auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
    auto config = stream_notify::loadConfig();
    if (stream_notify::showSettingsDialog(mainWindow, config)) stream_notify::saveConfig(config);
}

} // namespace

const char *obs_module_description(void)
{
    return obs_module_text("Module.Description");
}

bool obs_module_load(void)
{
    obs_frontend_add_event_callback(frontendEvent, nullptr);
    obs_frontend_add_tools_menu_item(obs_module_text("Menu.Open"), openSettings, nullptr);
    blog(LOG_INFO, "[stream-notify] loaded");
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(frontendEvent, nullptr);
}
