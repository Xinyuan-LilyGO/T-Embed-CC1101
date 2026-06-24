#pragma once

inline const char *stepStateText(const StepState state)
{
    switch (state) {
    case StepState::Pass:
        return "PASS";
    case StepState::Fail:
        return "FAIL";
    case StepState::Pending:
    default:
        return "WAIT";
    }
}

inline uint16_t stepStateColor(const StepState state)
{
    switch (state) {
    case StepState::Pass:
        return TFT_GREEN;
    case StepState::Fail:
        return TFT_RED;
    case StepState::Pending:
    default:
        return TFT_DARKGREY;
    }
}

inline uint16_t statusAccentColor()
{
    switch (summary.result) {
    case StepState::Pass:
        return TFT_GREEN;
    case StepState::Fail:
        return TFT_RED;
    case StepState::Pending:
    default:
        return TFT_YELLOW;
    }
}

inline uint16_t statusFillColor()
{
    switch (summary.result) {
    case StepState::Pass:
        return kColorPassBg;
    case StepState::Fail:
        return kColorFailBg;
    case StepState::Pending:
    default:
        return kColorPanel;
    }
}

inline String mountFrequencyText()
{
    if (!summary.mountedFrequency) {
        return "-";
    }
    return String(summary.mountedFrequency / 1000000UL) + " MHz";
}

inline String cardTypeText()
{
    return summary.hasCardDetails ? cardTypeStr(summary.cardType) : String("-");
}

inline String usageText()
{
    if (!summary.hasCardDetails) {
        return "-";
    }
    return String(static_cast<uint32_t>(summary.usedMB)) + " / " +
           String(static_cast<uint32_t>(summary.totalMB)) + " MB";
}

inline String rootEntriesText()
{
    return summary.rootMeasured ? String(summary.rootCount) + " entries" : String("-");
}

template <typename Canvas>
void drawHeader(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderHeight, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("SD Card Test", kUiMargin, 5, 2);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("Shared SPI", gfx.width() - kUiMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas &gfx)
{
    const int16_t y = gfx.height() - kFooterHeight;
    gfx.fillRect(0, y, gfx.width(), kFooterHeight, TFT_DARKGREY);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(summary.footer, kUiMargin, y + 4, 1);
}

template <typename Canvas>
void drawStatusPanel(Canvas &gfx)
{
    const uint16_t accent = statusAccentColor();
    const uint16_t fill = statusFillColor();

    gfx.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, fill);
    gfx.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, accent);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(accent, fill);
    gfx.drawString(summary.title, kStatusX + 10, kStatusY + 6, 2);

    gfx.fillRoundRect(kStatusX + kStatusW - 58, kStatusY + 7, 46, 16, 6, accent);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_BLACK, accent);
    gfx.drawString(stepStateText(summary.result), kStatusX + kStatusW - 35, kStatusY + 15, 1);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_LIGHTGREY, fill);
    gfx.drawString(summary.detail, kStatusX + 10, kStatusY + 24, 1);
}

template <typename Canvas>
void drawStepCard(Canvas &gfx, int16_t x, const char *label, const StepState state)
{
    const uint16_t accent = stepStateColor(state);

    gfx.fillRoundRect(x, kStepY, kStepW, kStepH, 6, kColorCard);
    gfx.drawRoundRect(x, kStepY, kStepW, kStepH, 6, accent);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_LIGHTGREY, kColorCard);
    gfx.drawString(label, x + 6, kStepY + 5, 1);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(accent, kColorCard);
    gfx.drawString(stepStateText(state), x + kStepW - 6, kStepY + 5, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawMetricCard(Canvas &gfx, int16_t x, int16_t y,
                    const char *label, const String &value, uint16_t borderColor)
{
    gfx.fillRoundRect(x, y, kMetricW, kMetricH, 6, kColorCard);
    gfx.drawRoundRect(x, y, kMetricW, kMetricH, 6, borderColor);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_CYAN, kColorCard);
    gfx.drawString(label, x + 8, y + 5, 1);

    gfx.setTextColor(TFT_WHITE, kColorCard);
    gfx.drawString(value, x + 52, y + 5, 1);
}

template <typename Canvas>
void drawUi(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kColorBg);

    drawHeader(gfx);
    drawStatusPanel(gfx);

    drawStepCard(gfx, kUiMargin, "Mount", summary.mount);
    drawStepCard(gfx, kUiMargin + (kStepW + kStepGap), "Write", summary.write);
    drawStepCard(gfx, kUiMargin + (kStepW + kStepGap) * 2, "Read", summary.read);
    drawStepCard(gfx, kUiMargin + (kStepW + kStepGap) * 3, "Result", summary.result);

    drawMetricCard(gfx, kMetricLeftX, kMetricTopY, "SPI", mountFrequencyText(), kColorPanelEdge);
    drawMetricCard(gfx, kMetricRightX, kMetricTopY, "Type", cardTypeText(), kColorPanelEdge);
    drawMetricCard(gfx, kMetricLeftX, kMetricBottomY, "Usage", usageText(), kColorPanelEdge);
    drawMetricCard(gfx, kMetricRightX, kMetricBottomY, "Root", rootEntriesText(), kColorPanelEdge);

    drawFooter(gfx);
}
