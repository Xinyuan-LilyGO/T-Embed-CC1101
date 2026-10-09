#pragma once

#include "ble_uart.h"

namespace page_ble {

namespace {

constexpr uint32_t kFrameMs = 80;
constexpr char kDeviceName[] = "T-Embed-BLE-UART";

bool backFocused = false;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
uint32_t lastStatusSeq = 0;
int32_t encSnapshot = 0;
factory_ble::Status status = {};

String clipLine(String text, const uint8_t maxChars)
{
    if (text.length() <= maxChars || maxChars < 4) {
        return text;
    }
    return text.substring(0, maxChars - 3) + "...";
}

const char* eventLabel(const uint8_t event)
{
    switch (event) {
        case factory_ble::EventAdvertising:   return "Advertising";
        case factory_ble::EventPairing:      return "Pairing";
        case factory_ble::EventBonded:       return "Bonded";
        case factory_ble::EventConnected:    return "Connected";
        case factory_ble::EventDisconnected: return "Disconnected";
        case factory_ble::EventAuthFailed:   return "Auth failed";
        case factory_ble::EventClearingBonds:return "Clearing bonds";
        case factory_ble::EventIdle:
        default:                             return "Idle";
    }
}

const char* statusBadgeText()
{
    if (!status.initialized) {
        return "FAIL";
    }
    if (status.authInProgress) {
        return "PAIR";
    }
    if (status.connected && status.rxCount > 0 && status.txCount > 0) {
        return "PASS";
    }
    if (status.connected) {
        return "LINK";
    }
    if (status.advertising) {
        return "ADV";
    }
    if (status.bonded) {
        return "BOND";
    }
    return "IDLE";
}

uint16_t statusBadgeColor()
{
    if (!status.initialized || status.lastEvent == factory_ble::EventAuthFailed) {
        return TFT_RED;
    }
    if (status.connected && status.rxCount > 0 && status.txCount > 0) {
        return TFT_GREEN;
    }
    if (status.authInProgress || status.lastEvent == factory_ble::EventClearingBonds) {
        return TFT_YELLOW;
    }
    if (status.connected || status.advertising) {
        return TFT_CYAN;
    }
    return TFT_ORANGE;
}

template <typename Canvas>
void drawBackButton(Canvas& gfx, const bool selected)
{
    const int16_t w = 58;
    const int16_t h = 14;
    const int16_t x = gfx.width() - w - 6;
    const int16_t y = gfx.height() - kFooterH + 2;
    const uint16_t bg = selected ? TFT_WHITE : TFT_DARKGREY;
    const uint16_t fg = selected ? TFT_BLACK : TFT_LIGHTGREY;

    gfx.fillRoundRect(x, y, w, h, 5, bg);
    gfx.drawRoundRect(x, y, w, h, 5, selected ? TFT_YELLOW : 0x52AA);
    gfx.setTextColor(fg, bg);
    gfx.drawCentreString("BACK", x + w / 2, y + 3, 1);
}

template <typename Canvas>
void drawHeader(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderH, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("Bluetooth BLE", kMargin, 5, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("Factory", gfx.width() - kMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas& gfx)
{
    const int16_t y = gfx.height() - kFooterH;
    gfx.fillRect(0, y, gfx.width(), kFooterH, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(backFocused ? "BOOT=back" : "BOOT=clear  USER=adv", kMargin, y + 4, 1);
    drawBackButton(gfx, backFocused);
}

template <typename Canvas>
void drawStatusBadge(Canvas& gfx)
{
    const int16_t x = 8;
    const int16_t y = 34;
    const int16_t w = 72;
    const int16_t h = 20;
    const uint16_t color = statusBadgeColor();
    const uint16_t bg = color == TFT_GREEN ? kPassBg : (color == TFT_RED ? kFailBg : kUiPanel);

    gfx.fillRoundRect(x, y, w, h, 6, bg);
    gfx.drawRoundRect(x, y, w, h, 6, color);
    gfx.setTextColor(color, bg);
    gfx.drawCentreString(statusBadgeText(), x + w / 2, y + 5, 1);
}

template <typename Canvas>
void drawUi(Canvas& gfx)
{
    gfx.fillScreen(kUiBg);
    drawHeader(gfx);
    drawFooter(gfx);
    drawStatusBadge(gfx);

    gfx.setTextColor(TFT_WHITE, kUiBg);
    gfx.drawString(eventLabel(status.lastEvent), 88, 38, 2);

    gfx.setTextColor(kUiMuted, kUiBg);
    gfx.drawString(String("Name: ") + kDeviceName, 8, 62, 1);

    char line[64];
    snprintf(line,
             sizeof(line),
             "PIN: %06lu   Bond: %u",
             static_cast<unsigned long>(status.passkey),
             static_cast<unsigned>(status.bondCount));
    gfx.setTextColor(TFT_YELLOW, kUiBg);
    gfx.drawString(line, 8, 75, 1);

    gfx.setTextColor(TFT_CYAN, kUiBg);
    gfx.drawString(String("Local: ") + clipLine(status.localAddress, 22), 8, 88, 1);
    gfx.setTextColor(status.connected ? TFT_GREEN : TFT_LIGHTGREY, kUiBg);
    gfx.drawString(String("Peer:  ") + clipLine(status.peerAddress[0] ? status.peerAddress : "-", 22), 8, 101, 1);

    gfx.setTextColor(TFT_WHITE, kUiBg);
    snprintf(line,
             sizeof(line),
             "RX: %lu   TX: %lu",
             static_cast<unsigned long>(status.rxCount),
             static_cast<unsigned long>(status.txCount));
    gfx.drawString(line, 8, 116, 2);

    gfx.setTextColor(TFT_LIGHTGREY, kUiBg);
    gfx.drawString(String("Last: ") + clipLine(status.lastPayload, 42), 8, 137, 1);
}

}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    lastStatusSeq = 0;
    encSnapshot = g.encRaw;
    factory_ble::begin();
    factory_ble::getStatus(&status);
    lastStatusSeq = status.updateSeq;
}

void update()
{
    factory_ble::service();

    factory_ble::Status currentStatus = {};
    factory_ble::getStatus(&currentStatus);
    if (currentStatus.updateSeq != lastStatusSeq) {
        status = currentStatus;
        lastStatusSeq = currentStatus.updateSeq;
        screenDirty = true;
    }

    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    if (takeUserButton()) {
        (void)factory_ble::restartAdvertising();
        screenDirty = true;
    }

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        (void)factory_ble::clearBonds();
        screenDirty = true;
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    gSubPageGfx.beginFrame();
    drawUi(tft);
    gSubPageGfx.endFrame();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    factory_ble::stop();
}

}  // namespace page_ble
