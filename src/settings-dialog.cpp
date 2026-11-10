/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settings-dialog.hpp"

#include <obs-module.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace stream_notify {

namespace {

QString text(const char *key)
{
    return QString::fromUtf8(obs_module_text(key));
}

QLabel *hint(const char *key)
{
    auto *label = new QLabel(text(key));
    label->setWordWrap(true);
    return label;
}

} // namespace

bool showSettingsDialog(QWidget *parent, Config &config)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(text("Settings.Title"));
    dialog.setMinimumWidth(480);

    auto *token = new QLineEdit(config.token);
    token->setEchoMode(QLineEdit::Password);
    auto *chat = new QLineEdit(config.chat);
    chat->setPlaceholderText(text("Telegram.ChannelPlaceholder"));

    auto *form = new QFormLayout;
    form->addRow(text("Telegram.BotToken"), token);
    form->addRow(QString(), hint("Telegram.BotHint"));
    form->addRow(text("Telegram.Channel"), chat);
    form->addRow(QString(), hint("Telegram.ChannelHint"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return false;
    config.token = token->text().trimmed();
    config.chat = chat->text().trimmed();
    return true;
}

} // namespace stream_notify
