/*
 * Stream Notify
 * Copyright (C) 2026 Stream Notify contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "message-format.hpp"
#include "telegram-api.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char *message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
}

void testRenderPostText()
{
    using stream_notify::renderPostText;
    expect(renderPostText("Live!\n{url}", {{"url", "https://twitch.tv/me"}}) == "Live!\nhttps://twitch.tv/me",
           "placeholder is replaced");
    expect(renderPostText("Live!\n{title}\nWatch: {url}", {{"title", ""}, {"url", "https://x"}}) ==
               "Live!\nWatch: https://x",
           "line with only an empty placeholder is removed");
    expect(renderPostText("Live! {title}", {{"title", ""}}) == "Live!", "empty inline placeholder is cleared");
    expect(renderPostText("{unknown}", {}) == "{unknown}", "unknown placeholder is kept");
}

void testReplyMarkup()
{
    using stream_notify::telegram::replyMarkup;
    const QJsonObject root = QJsonDocument::fromJson(replyMarkup("Watch", "https://x").toUtf8()).object();
    const QJsonArray keyboard = root.value("inline_keyboard").toArray();
    expect(keyboard.size() == 1 && keyboard.at(0).toArray().size() == 1, "keyboard has one row with one button");
    const QJsonObject button = keyboard.at(0).toArray().at(0).toObject();
    expect(button.value("text") == "Watch" && button.value("url") == "https://x", "button keeps text and url");
    expect(replyMarkup("", "https://x").isEmpty(), "empty button text disables markup");
    expect(replyMarkup("Watch", "").isEmpty(), "empty url disables markup");
}

void testSendMessageBody()
{
    using namespace stream_notify;
    const QUrlQuery form(QString::fromUtf8(telegram::sendMessageBody("@chan", Post{"Hi & bye", "Watch", "https://x", {}})));
    expect(form.queryItemValue("chat_id") == "@chan", "chat id is sent");
    expect(form.queryItemValue("text", QUrl::FullyDecoded) == "Hi & bye", "text survives form encoding");
    expect(form.hasQueryItem("reply_markup"), "button adds reply markup");
    const QUrlQuery plain(QString::fromUtf8(telegram::sendMessageBody("@chan", Post{"Hi", "", "", {}})));
    expect(!plain.hasQueryItem("reply_markup"), "no button means no reply markup");
}

void testEditAndDeleteBodies()
{
    using namespace stream_notify;
    const QUrlQuery edit(QString::fromUtf8(telegram::editMessageBody("@chan", 42, Post{"Ended", "", "", {}})));
    expect(edit.queryItemValue("message_id") == "42" && edit.queryItemValue("text") == "Ended",
           "edit body has message id and new text");
    const QUrlQuery caption(QString::fromUtf8(telegram::editCaptionBody("@chan", 42, Post{"Ended", "", "", {}})));
    expect(caption.queryItemValue("caption") == "Ended" && !caption.hasQueryItem("text"),
           "caption edit uses the caption field");
    const QUrlQuery remove(QString::fromUtf8(telegram::deleteMessageBody("@chan", 42)));
    expect(remove.queryItemValue("chat_id") == "@chan" && remove.queryItemValue("message_id") == "42",
           "delete body has chat and message id");
}

void testLimitText()
{
    using stream_notify::telegram::limitText;
    expect(limitText("short", 10) == "short", "short text is unchanged");
    const QString cut = limitText(QString(20, 'a'), 10);
    expect(cut.size() == 10 && cut.endsWith(QChar(0x2026)), "long text is cut with an ellipsis");
    const QString emoji = QString(8, 'a') + QString::fromUtf8("\xF0\x9F\x94\xB4") + "tail";
    const QString safe = limitText(emoji, 10);
    expect(!safe.at(safe.size() - 2).isHighSurrogate(), "emoji is not split in half");
}

void testHideToken()
{
    using stream_notify::telegram::hideToken;
    expect(hideToken("Error transferring https://api.telegram.org/bot123:abc/sendMessage", "123:abc") ==
               "Error transferring https://api.telegram.org/bot<token>/sendMessage",
           "token is removed from error text");
    expect(hideToken("timeout", "") == "timeout", "empty token leaves text unchanged");
}

} // namespace

int main()
{
    testRenderPostText();
    testReplyMarkup();
    testSendMessageBody();
    testEditAndDeleteBodies();
    testLimitText();
    testHideToken();
    if (failures == 0) std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}
