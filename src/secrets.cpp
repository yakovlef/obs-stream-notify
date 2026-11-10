/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "secrets.hpp"

#include <obs-module.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

namespace stream_notify {

namespace {
const QString prefix = QStringLiteral("dpapi:");
}

QString protectSecret(const QString &plain)
{
#ifdef _WIN32
    if (plain.isEmpty()) return {};
    QByteArray bytes = plain.toUtf8();
    DATA_BLOB input{static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE *>(bytes.data())};
    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"Stream Notify", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        blog(LOG_WARNING, "[stream-notify] Could not encrypt secret");
        return {};
    }
    const QByteArray encrypted(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return prefix + QString::fromLatin1(encrypted.toBase64());
#else
    return plain;
#endif
}

QString unprotectSecret(const QString &stored)
{
    if (!stored.startsWith(prefix)) return stored;
#ifdef _WIN32
    QByteArray bytes = QByteArray::fromBase64(stored.mid(prefix.size()).toLatin1());
    DATA_BLOB input{static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE *>(bytes.data())};
    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        blog(LOG_WARNING, "[stream-notify] Could not decrypt secret");
        return {};
    }
    const QString plain = QString::fromUtf8(reinterpret_cast<const char *>(output.pbData), static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return plain;
#else
    return {};
#endif
}

} // namespace stream_notify
