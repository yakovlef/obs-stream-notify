/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "message-format.hpp"

#include <QStringList>

namespace stream_notify {

QString renderPostText(const QString &postTemplate, const QHash<QString, QString> &values)
{
    QStringList lines;
    for (QString line : postTemplate.split('\n')) {
        bool emptyPlaceholderOnly = false;
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            const QString placeholder = '{' + it.key() + '}';
            if (it.value().isEmpty() && line.trimmed() == placeholder) emptyPlaceholderOnly = true;
            line.replace(placeholder, it.value());
        }
        if (!emptyPlaceholderOnly) lines.append(line);
    }
    return lines.join('\n').trimmed();
}

} // namespace stream_notify
