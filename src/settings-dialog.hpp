/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "config.hpp"

class QWidget;

namespace stream_notify {

bool showSettingsDialog(QWidget *parent, Config &config);

} // namespace stream_notify
