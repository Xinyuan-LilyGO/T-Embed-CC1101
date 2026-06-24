#pragma once

constexpr int16_t kUiMargin = 8;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;

constexpr int16_t kTileY = 32;
constexpr int16_t kTileW = 96;
constexpr int16_t kTileH = 24;
constexpr int16_t kTileGap = 8;

constexpr int16_t kPanelX = 8;
constexpr int16_t kPanelY = 64;
constexpr int16_t kPanelW = 304;
constexpr int16_t kPanelH = 44;

constexpr int16_t kActionY = 116;
constexpr int16_t kActionW = 148;
constexpr int16_t kActionH = 26;

inline uint16_t stateAccent()
{
    switch (gState) {
    case State::WAIT_SD:
        return TFT_ORANGE;
    case State::RECORDING:
        return TFT_RED;
    case State::READY:
        return TFT_GREEN;
    case State::PLAYING:
        return TFT_CYAN;
    case State::ERROR:
        return TFT_RED;
    default:
        return TFT_DARKGREY;
    }
}

inline const char *stateTitle()
{
    switch (gState) {
    case State::WAIT_SD:
        return "Insert SD card to continue";
    case State::RECORDING:
        return "Recording 10 second WAV";
    case State::READY:
        return "Recording saved to SD";
    case State::PLAYING:
        return "Playing back /record.wav";
    case State::ERROR:
        return gErrorTitle[0] ? gErrorTitle : "Audio test error";
    default:
        return "Audio record test";
    }
}

inline const char *stateHint()
{
    switch (gState) {
    case State::WAIT_SD:
        return "The test will auto-start once a card is detected.";
    case State::RECORDING:
        return "Speak near the microphone. USER restarts immediately.";
    case State::READY:
        return "Rotate encoder to PLAY or REC, then press encoder key.";
    case State::PLAYING:
        return "Rotate encoder to STOP or REC, then press encoder key.";
    case State::ERROR:
        return gErrorDetail[0] ? gErrorDetail : "Press USER to try again.";
    default:
        return "";
    }
}

inline const char *footerText()
{
    switch (gState) {
    case State::WAIT_SD:
        return "Waiting for SD card";
    case State::RECORDING:
        return "USER: restart recording";
    case State::READY:
    case State::PLAYING:
        return "ENC: select  KEY: run  USER: rerecord";
    case State::ERROR:
        return "USER: retry recording";
    default:
        return "";
    }
}

inline const char *cardStateText(CardState state)
{
    switch (state) {
    case CardState::ACTIVE:
        return "RUN";
    case CardState::PASS:
        return "OK";
    case CardState::FAIL:
        return "FAIL";
    case CardState::WAIT:
    default:
        return "WAIT";
    }
}

inline uint16_t cardBorderColor(CardState state)
{
    switch (state) {
    case CardState::ACTIVE:
        return stateAccent();
    case CardState::PASS:
        return TFT_GREEN;
    case CardState::FAIL:
        return TFT_RED;
    case CardState::WAIT:
    default:
        return kColorPanelEdge;
    }
}

inline uint16_t cardFillColor(CardState state)
{
    switch (state) {
    case CardState::ACTIVE:
        return kColorActiveBg;
    case CardState::PASS:
        return kColorPassBg;
    case CardState::FAIL:
        return kColorFailBg;
    case CardState::WAIT:
    default:
        return kColorCard;
    }
}

inline String sdMetricValue()
{
    if (!gSdMounted) {
        return String("NO CARD");
    }

    if (gCardTotalMB == 0) {
        return String("READY");
    }

    return String(static_cast<uint32_t>(gCardUsedMB)) + "/" +
           String(static_cast<uint32_t>(gCardTotalMB)) + "M";
}

inline String fileMetricValue()
{
    if (!gSdMounted || !gHasRecording) {
        return String("--");
    }
    return formatBytes(gRecordedFileSize);
}

inline String infoMetricValue()
{
    if (gState == State::RECORDING) {
        return String(gRecordRms);
    }
    if (gSdMounted && gMountedFrequency) {
        return String(gMountedFrequency / 1000000UL) + "M";
    }
    return String("--");
}

inline String timeMetricValue()
{
    if (gState == State::RECORDING) {
        return String(static_cast<float>(recordElapsedMs()) / 1000.0f, 1) + "/10s";
    }
    return String(kRecordDurationSec) + "s";
}

inline const char *primaryActionLabel()
{
    switch (gState) {
    case State::READY:
        return "PLAY";
    case State::PLAYING:
        return "STOP";
    case State::ERROR:
        return "REC";
    case State::RECORDING:
        return "REC";
    case State::WAIT_SD:
    default:
        return "WAIT";
    }
}

inline const char *primaryActionDetail()
{
    switch (gState) {
    case State::READY:
        return "play latest wav";
    case State::PLAYING:
        return "stop playback";
    case State::ERROR:
        return "record again";
    case State::RECORDING:
        return "capturing now";
    case State::WAIT_SD:
    default:
        return "insert SD card";
    }
}

inline const char *secondaryActionLabel()
{
    switch (gState) {
    case State::READY:
    case State::PLAYING:
    case State::ERROR:
        return "REC";
    case State::RECORDING:
        return "USER";
    case State::WAIT_SD:
    default:
        return "AUTO";
    }
}

inline const char *secondaryActionDetail()
{
    switch (gState) {
    case State::READY:
    case State::PLAYING:
    case State::ERROR:
        return "new 10s take";
    case State::RECORDING:
        return "restart take";
    case State::WAIT_SD:
    default:
        return "auto detect";
    }
}

inline bool actionsInteractive()
{
    return gState == State::READY || gState == State::PLAYING || gState == State::ERROR;
}

inline uint16_t progressFillColor()
{
    if (gState == State::RECORDING) {
        return TFT_RED;
    }
    if (gState == State::READY) {
        return TFT_GREEN;
    }
    if (gState == State::PLAYING) {
        return TFT_CYAN;
    }
    return TFT_DARKGREY;
}

template <typename Canvas>
void drawHeader(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderHeight, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("Record Audio Test", kUiMargin, 5, 2);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("16kHz / 10s", gfx.width() - kUiMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas &gfx)
{
    const int16_t y = gfx.height() - kFooterHeight;
    gfx.fillRect(0, y, gfx.width(), kFooterHeight, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(footerText(), kUiMargin, y + 4, 1);
}

template <typename Canvas>
void drawStateCard(Canvas &gfx, int16_t x, const char *label, CardState state)
{
    const uint16_t fill = cardFillColor(state);
    const uint16_t border = cardBorderColor(state);

    gfx.fillRoundRect(x, kTileY, kTileW, kTileH, 6, fill);
    gfx.drawRoundRect(x, kTileY, kTileW, kTileH, 6, border);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, fill);
    gfx.drawString(label, x + 8, kTileY + 5, 2);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(border, fill);
    gfx.drawString(cardStateText(state), x + kTileW - 8, kTileY + 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawStateRow(Canvas &gfx)
{
    drawStateCard(gfx, kUiMargin, "SD", sdCardState());
    drawStateCard(gfx, kUiMargin + kTileW + kTileGap, "REC", recordCardState());
    drawStateCard(gfx, kUiMargin + (kTileW + kTileGap) * 2, "PLAY", playCardState());
}

template <typename Canvas>
void drawProgressBar(Canvas &gfx)
{
    const int16_t x = kPanelX + 188;
    const int16_t y = kPanelY + 10;
    const int16_t w = 104;
    const int16_t h = 8;

    gfx.drawRect(x, y, w, h, kColorPanelEdge);
    gfx.fillRect(x + 1, y + 1, w - 2, h - 2, kColorCard);

    uint16_t percent = 0;
    if (gState == State::RECORDING) {
        percent = recordProgressPercent();
    } else if (gHasRecording) {
        percent = 100;
    }

    const int16_t fillW = static_cast<int16_t>(((w - 2) * percent) / 100U);
    if (fillW > 0) {
        gfx.fillRect(x + 1, y + 1, fillW, h - 2, progressFillColor());
    }
}

template <typename Canvas>
void drawMetricPair(Canvas &gfx, int16_t x, int16_t y, const char *label, const String &value, uint16_t valueColor)
{
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_LIGHTGREY, kColorPanel);
    gfx.drawString(label, x, y, 1);

    gfx.setTextColor(valueColor, kColorPanel);
    gfx.drawString(value, x + 24, y, 1);
}

template <typename Canvas>
void drawPanel(Canvas &gfx)
{
    gfx.fillRoundRect(kPanelX, kPanelY, kPanelW, kPanelH, 8, kColorPanel);
    gfx.drawRoundRect(kPanelX, kPanelY, kPanelW, kPanelH, 8, stateAccent());

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, kColorPanel);
    gfx.drawString(stateTitle(), kPanelX + 10, kPanelY + 7, 2);

    drawProgressBar(gfx);

    gfx.setTextColor(TFT_LIGHTGREY, kColorPanel);
    gfx.drawString(stateHint(), kPanelX + 10, kPanelY + 24, 1);

    drawMetricPair(gfx, kPanelX + 10, kPanelY + 34, "SD", sdMetricValue(), TFT_CYAN);
    drawMetricPair(gfx, kPanelX + 104, kPanelY + 34, "FILE", fileMetricValue(), TFT_WHITE);

    const uint16_t infoColor = (gState == State::RECORDING) ? TFT_YELLOW : stateAccent();
    const char *infoLabel = (gState == State::RECORDING) ? "LVL" : "TIME";
    const String infoValue = (gState == State::RECORDING) ? infoMetricValue() : timeMetricValue();
    drawMetricPair(gfx, kPanelX + 214, kPanelY + 34, infoLabel, infoValue, infoColor);
}

template <typename Canvas>
void drawActionCard(Canvas &gfx, int16_t x, const char *label, const char *detail, bool selected)
{
    const bool interactive = actionsInteractive();
    const uint16_t border = selected && interactive ? stateAccent() : kColorPanelEdge;
    const uint16_t fill = selected && interactive ? kColorActiveBg : kColorCard;

    gfx.fillRoundRect(x, kActionY, kActionW, kActionH, 7, fill);
    gfx.drawRoundRect(x, kActionY, kActionW, kActionH, 7, border);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(interactive ? TFT_WHITE : TFT_LIGHTGREY, fill);
    gfx.drawString(label, x + 8, kActionY + 5, 2);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(interactive ? border : TFT_DARKGREY, fill);
    gfx.drawString(detail, x + kActionW - 8, kActionY + 8, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawActions(Canvas &gfx)
{
    drawActionCard(gfx, kUiMargin, primaryActionLabel(), primaryActionDetail(), gMenuIndex == 0U);
    drawActionCard(gfx, kUiMargin + kActionW + kTileGap,
                   secondaryActionLabel(),
                   secondaryActionDetail(),
                   gMenuIndex == 1U);
}

template <typename Canvas>
void drawUi(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kColorBg);
    drawHeader(gfx);
    drawStateRow(gfx);
    drawPanel(gfx);
    drawActions(gfx);
    drawFooter(gfx);
}
