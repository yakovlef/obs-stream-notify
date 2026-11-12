/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settings-dialog.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
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
    auto *url = new QLineEdit(config.url);
    url->setPlaceholderText("https://twitch.tv/your_channel");
    auto *button = new QLineEdit(config.button);
    auto *delay = new QSpinBox;
    delay->setRange(0, 3600);
    delay->setSuffix(text("Delay.Suffix"));
    delay->setValue(config.delay);
    auto *body = new QPlainTextEdit(config.text);
    body->setPlaceholderText(text("Post.TextPlaceholder"));
    auto *endAction = new QComboBox;
    endAction->addItem(text("End.Edit"), static_cast<int>(EndAction::Edit));
    endAction->addItem(text("End.Delete"), static_cast<int>(EndAction::Delete));
    endAction->addItem(text("End.Keep"), static_cast<int>(EndAction::Keep));
    endAction->setCurrentIndex(endAction->findData(static_cast<int>(config.endAction)));
    auto *endText = new QPlainTextEdit(config.endText);
    endText->setMaximumHeight(80);
    auto updateEndText = [endAction, endText] {
        endText->setEnabled(endAction->currentData().toInt() == static_cast<int>(EndAction::Edit));
    };
    QObject::connect(endAction, &QComboBox::currentIndexChanged, endText, updateEndText);
    updateEndText();

    auto *form = new QFormLayout;
    form->addRow(text("Telegram.BotToken"), token);
    form->addRow(QString(), hint("Telegram.BotHint"));
    form->addRow(text("Telegram.Channel"), chat);
    form->addRow(QString(), hint("Telegram.ChannelHint"));
    form->addRow(text("Post.StreamUrl"), url);
    form->addRow(text("Post.ButtonText"), button);
    form->addRow(text("Post.Delay"), delay);
    form->addRow(QString(), hint("Post.DelayHint"));
    form->addRow(text("Post.Text"), body);
    form->addRow(QString(), hint("Post.Placeholders"));
    form->addRow(text("End.Action"), endAction);
    form->addRow(text("End.Text"), endText);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return false;
    config.token = token->text().trimmed();
    config.chat = chat->text().trimmed();
    config.url = url->text().trimmed();
    config.button = button->text().trimmed();
    config.delay = delay->value();
    config.text = body->toPlainText();
    config.endAction = static_cast<EndAction>(endAction->currentData().toInt());
    config.endText = endText->toPlainText();
    return true;
}

} // namespace stream_notify
