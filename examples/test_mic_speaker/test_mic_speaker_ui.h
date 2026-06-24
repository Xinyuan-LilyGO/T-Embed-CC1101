#pragma once

enum class StageStatus : uint8_t {
    PENDING = 0,
    ACTIVE,
    PASS,
    FAIL,
};

struct StageStyle {
    uint16_t fill;
    uint16_t border;
    uint16_t label;
    uint16_t tag;
    const char *tagText;
};

inline uint16_t stateAccent()
{
    switch (gState) {
    case State::INIT_MIC:
    case State::MIC_TEST:
        return TFT_CYAN;
    case State::INIT_SPK:
    case State::SPEAKER_TEST:
        return TFT_YELLOW;
    case State::INIT_LOOPBACK:
    case State::LOOPBACK_TEST:
        return TFT_ORANGE;
    case State::DONE_PASS:
        return TFT_GREEN;
    case State::DONE_FAIL:
        return TFT_RED;
    default:
        return TFT_DARKGREY;
    }
}

inline const char *stateTitle()
{
    switch (gState) {
    case State::INIT_MIC:
        return "Initializing microphone";
    case State::MIC_TEST:
        return "Microphone capture test";
    case State::INIT_SPK:
        return "Preparing speaker output";
    case State::SPEAKER_TEST:
        return "Speaker tone sweep";
    case State::INIT_LOOPBACK:
        return "Configuring loopback";
    case State::LOOPBACK_TEST:
        return "Mic to speaker loopback";
    case State::DONE_PASS:
        return "Audio self-check passed";
    case State::DONE_FAIL:
        return "Audio self-check failed";
    default:
        return "Audio self-check";
    }
}

inline const char *stateHint()
{
    switch (gState) {
    case State::INIT_MIC:
        return "Starting the PDM mic path...";
    case State::MIC_TEST:
        return "Make some noise close to the microphone.";
    case State::INIT_SPK:
        return "Enabling I2S speaker output...";
    case State::SPEAKER_TEST:
        return "Listen for the three test tones.";
    case State::INIT_LOOPBACK:
        return "Bringing mic and speaker online together...";
    case State::LOOPBACK_TEST:
        return "Speak now. You should hear delayed playback.";
    case State::DONE_PASS:
        return "All three stages completed successfully.";
    case State::DONE_FAIL:
        return "Check mic, speaker and power path, then rerun.";
    default:
        return "";
    }
}

inline const char *footerText()
{
    if (gState == State::DONE_PASS || gState == State::DONE_FAIL) {
        return "USR key: rerun test";
    }
    return "Audio self-test running";
}

inline uint32_t activeStageDurationMs()
{
    switch (gState) {
    case State::MIC_TEST:
        return kMicTestDurationMs;
    case State::SPEAKER_TEST:
        return kSpkFreqDwellMs * 3U;
    case State::LOOPBACK_TEST:
        return kLoopbackDurationMs;
    default:
        return 0;
    }
}

inline uint32_t activeStageElapsedMs()
{
    const uint32_t duration = activeStageDurationMs();
    if (duration == 0) {
        return 0;
    }

    const uint32_t elapsed = millis() - gStateEnterMs;
    return elapsed > duration ? duration : elapsed;
}

inline bool hasInputMeter()
{
    return gState == State::MIC_TEST ||
           gState == State::LOOPBACK_TEST ||
           gState == State::DONE_PASS ||
           gState == State::DONE_FAIL;
}

inline StageStatus micStageStatus()
{
    if (gState == State::INIT_MIC || gState == State::MIC_TEST) {
        return StageStatus::ACTIVE;
    }
    if (!gMicStarted) {
        return StageStatus::PENDING;
    }
    return gMicOk ? StageStatus::PASS : StageStatus::FAIL;
}

inline StageStatus spkStageStatus()
{
    if (gState == State::INIT_SPK || gState == State::SPEAKER_TEST) {
        return StageStatus::ACTIVE;
    }
    if (!gSpkStarted) {
        return StageStatus::PENDING;
    }
    return gSpkOk ? StageStatus::PASS : StageStatus::FAIL;
}

inline StageStatus loopStageStatus()
{
    if (gState == State::INIT_LOOPBACK || gState == State::LOOPBACK_TEST) {
        return StageStatus::ACTIVE;
    }
    if (!gLoopStarted) {
        return StageStatus::PENDING;
    }
    return gLoopOk ? StageStatus::PASS : StageStatus::FAIL;
}

inline StageStyle stageStyle(StageStatus status, uint16_t accent)
{
    switch (status) {
    case StageStatus::ACTIVE:
        return {kColorCard, accent, TFT_WHITE, accent, "RUN"};
    case StageStatus::PASS:
        return {kColorPassBg, TFT_GREEN, TFT_WHITE, TFT_GREEN, "PASS"};
    case StageStatus::FAIL:
        return {kColorFailBg, TFT_RED, TFT_WHITE, TFT_RED, "FAIL"};
    case StageStatus::PENDING:
    default:
        return {kColorCard, kColorPanelEdge, TFT_LIGHTGREY, TFT_DARKGREY, "WAIT"};
    }
}

inline uint16_t meterBarColor(uint16_t width, uint16_t fullWidth)
{
    if (width < fullWidth * 60 / 100) {
        return TFT_GREEN;
    }
    if (width < fullWidth * 85 / 100) {
        return TFT_YELLOW;
    }
    return TFT_RED;
}

inline const char *levelLabel(uint16_t rms)
{
    if (rms < 100) {
        return "SILENT";
    }
    if (rms < 500) {
        return "LOW";
    }
    if (rms < 2000) {
        return "MED";
    }
    return "LOUD";
}

inline uint16_t levelColor(uint16_t rms)
{
    if (rms < 100) {
        return TFT_DARKGREY;
    }
    if (rms < 500) {
        return TFT_CYAN;
    }
    if (rms < 2000) {
        return TFT_GREEN;
    }
    return TFT_RED;
}

template <typename Canvas>
void drawHeader(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderHeight, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("Mic & Speaker Test", kUiMargin, 5, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("16 kHz audio", gfx.width() - kUiMargin, 7, 1);
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
void drawStageCard(Canvas &gfx, int16_t x, const char *label, StageStatus status, uint16_t accent)
{
    const StageStyle style = stageStyle(status, accent);
    gfx.fillRoundRect(x, kStageCardY, kStageCardW, kStageCardH, 6, style.fill);
    gfx.drawRoundRect(x, kStageCardY, kStageCardW, kStageCardH, 6, style.border);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(style.label, style.fill);
    gfx.drawString(label, x + 8, kStageCardY + 6, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(style.tag, style.fill);
    gfx.drawString(style.tagText, x + kStageCardW - 8, kStageCardY + 9, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawStageRow(Canvas &gfx)
{
    drawStageCard(gfx, kUiMargin, "MIC", micStageStatus(), TFT_CYAN);
    drawStageCard(gfx, kUiMargin + kStageCardW + kStageGap, "SPK", spkStageStatus(), TFT_YELLOW);
    drawStageCard(gfx, kUiMargin + (kStageCardW + kStageGap) * 2, "LOOP", loopStageStatus(), TFT_ORANGE);
}

template <typename Canvas>
void drawProgressBar(Canvas &gfx, int16_t x, int16_t y, int16_t w, int16_t h)
{
    gfx.drawRect(x, y, w, h, kColorPanelEdge);
    gfx.fillRect(x + 1, y + 1, w - 2, h - 2, kColorMeterBg);

    const uint32_t duration = activeStageDurationMs();
    if (duration == 0 || w <= 2 || h <= 2) {
        return;
    }

    const int16_t fillW = static_cast<int16_t>((static_cast<uint32_t>(w - 2) * activeStageElapsedMs()) / duration);
    if (fillW > 0) {
        gfx.fillRect(x + 1, y + 1, fillW, h - 2, stateAccent());
    }
}

template <typename Canvas>
void drawVuBar(Canvas &gfx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t rms, uint16_t peak)
{
    gfx.drawRect(x - 1, y - 1, w + 2, h + 2, kColorPanelEdge);
    gfx.fillRect(x, y, w, h, kColorMeterBg);

    int16_t fillW = static_cast<int16_t>((static_cast<uint32_t>(rms) * w) / 8000U);
    if (fillW > w) {
        fillW = w;
    }

    if (fillW > 0) {
        gfx.fillRect(x, y, fillW, h, meterBarColor(fillW, w));
    }

    int16_t peakX = static_cast<int16_t>((static_cast<uint32_t>(peak) * w) / 8000U);
    if (peakX > w) {
        peakX = w;
    }
    if (peakX > 0) {
        gfx.drawFastVLine(x + peakX - 1, y, h, TFT_WHITE);
    }
}

template <typename Canvas>
void drawActivePanel(Canvas &gfx)
{
    gfx.fillRoundRect(kPanelX, kPanelY, kPanelW, kPanelH, 8, kColorPanel);
    gfx.drawRoundRect(kPanelX, kPanelY, kPanelW, kPanelH, 8, stateAccent());

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, kColorPanel);
    gfx.drawString(stateTitle(), kPanelX + 10, kPanelY + 7, 2);

    drawProgressBar(gfx, kPanelX + kPanelW - 108, kPanelY + 10, 96, 8);

    gfx.setTextColor(TFT_LIGHTGREY, kColorPanel);
    gfx.drawString(stateHint(), kPanelX + 10, kPanelY + 24, 1);

    if (gState == State::MIC_TEST || gState == State::LOOPBACK_TEST) {
        drawVuBar(gfx, kPanelX + 12, kPanelY + 36, kPanelW - 24, 10, gVuRms, gVuPeak);
        return;
    }

    if (gState == State::SPEAKER_TEST) {
        char freqBuf[16];
        snprintf(freqBuf, sizeof(freqBuf), "%.0f Hz", kToneFreqs[gFreqIndex]);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(TFT_YELLOW, kColorPanel);
        gfx.drawString(freqBuf, kPanelX + kPanelW / 2, kPanelY + 35, 4);
        gfx.setTextDatum(TL_DATUM);
        return;
    }

    if (gState == State::DONE_PASS || gState == State::DONE_FAIL) {
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(stateAccent(), kColorPanel);
        gfx.drawString(gState == State::DONE_PASS ? "PASS" : "FAIL",
                       kPanelX + kPanelW / 2,
                       kPanelY + 35,
                       4);
        gfx.setTextDatum(TL_DATUM);
    }
}

template <typename Canvas>
void drawMetricCard(Canvas &gfx, int16_t x, const char *title, const char *value,
                    uint16_t valueColor, uint16_t borderColor)
{
    gfx.fillRoundRect(x, kMetricY, kMetricW, kMetricH, 6, kColorCard);
    gfx.drawRoundRect(x, kMetricY, kMetricW, kMetricH, 6, borderColor);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_LIGHTGREY, kColorCard);
    gfx.drawString(title, x + 6, kMetricY + 5, 1);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(valueColor, kColorCard);
    gfx.drawString(value, x + kMetricW - 6, kMetricY + 4, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawMetricRow(Canvas &gfx)
{
    char rmsBuf[12];
    char peakBuf[12];
    char infoBuf[16];

    if (hasInputMeter()) {
        snprintf(rmsBuf, sizeof(rmsBuf), "%u", gVuRms);
        snprintf(peakBuf, sizeof(peakBuf), "%u", gVuPeak);
    } else {
        snprintf(rmsBuf, sizeof(rmsBuf), "--");
        snprintf(peakBuf, sizeof(peakBuf), "--");
    }

    uint16_t infoColor = TFT_DARKGREY;
    if (gState == State::SPEAKER_TEST) {
        snprintf(infoBuf, sizeof(infoBuf), "%.0fHz", kToneFreqs[gFreqIndex]);
        infoColor = TFT_YELLOW;
    } else if (gState == State::DONE_PASS) {
        snprintf(infoBuf, sizeof(infoBuf), "OK");
        infoColor = TFT_GREEN;
    } else if (gState == State::DONE_FAIL) {
        snprintf(infoBuf, sizeof(infoBuf), "CHECK");
        infoColor = TFT_RED;
    } else if (hasInputMeter()) {
        snprintf(infoBuf, sizeof(infoBuf), "%s", levelLabel(gVuRms));
        infoColor = levelColor(gVuRms);
    } else {
        snprintf(infoBuf, sizeof(infoBuf), "WAIT");
    }

    drawMetricCard(gfx, kUiMargin, "RMS", rmsBuf, TFT_WHITE, kColorPanelEdge);
    drawMetricCard(gfx, kUiMargin + kMetricW + kStageGap, "PEAK", peakBuf, TFT_WHITE, kColorPanelEdge);
    drawMetricCard(gfx,
                   kUiMargin + (kMetricW + kStageGap) * 2,
                   "INFO",
                   infoBuf,
                   infoColor,
                   stateAccent());
}

template <typename Canvas>
void drawUi(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kColorBg);
    drawHeader(gfx);
    drawStageRow(gfx);
    drawActivePanel(gfx);
    drawMetricRow(gfx);
    drawFooter(gfx);
}
