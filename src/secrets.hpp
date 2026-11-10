/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

namespace stream_notify {

QString protectSecret(const QString &plain);
QString unprotectSecret(const QString &stored);

} // namespace stream_notify
