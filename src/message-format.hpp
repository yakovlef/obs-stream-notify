/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QString>

namespace stream_notify {

QString resolveStreamUrl(const QString &customUrl, const QString &twitchLogin);
QString renderPostText(const QString &postTemplate, const QHash<QString, QString> &values);

} // namespace stream_notify
