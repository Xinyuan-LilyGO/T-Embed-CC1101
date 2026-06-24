#pragma once

template <typename Canvas>
void drawHeader(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderHeight, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("WS2812 Test", kUiMargin, 5, 2);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("NeoPixel", gfx.width() - kUiMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawStatusPanel(Canvas &gfx)
{
    const PatternDef &pattern = currentPattern();

    gfx.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, kColorPanel);
    gfx.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, pattern.accent);

    gfx.setTextColor(pattern.accent, kColorPanel);
    gfx.drawString(pattern.name, kStatusX + 10, kStatusY + 6, 2);

    char stepLabel[16];
    snprintf(stepLabel, sizeof(stepLabel), "%u/%u", currentPatternIndex + 1, kPatternCount);
    gfx.fillRoundRect(kStatusX + kStatusW - 52, kStatusY + 7, 40, 16, 6, pattern.accent);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_BLACK, pattern.accent);
    gfx.drawString(stepLabel, kStatusX + kStatusW - 32, kStatusY + 15, 1);
    gfx.setTextDatum(TL_DATUM);

    gfx.setTextColor(TFT_LIGHTGREY, kColorPanel);
    gfx.drawString(pattern.detail, kStatusX + 10, kStatusY + 24, 1);
}

template <typename Canvas>
void drawPreviewPanel(Canvas &gfx)
{
    gfx.fillRoundRect(kPreviewX, kPreviewY, kPreviewW, kPreviewH, 8, kColorCard);
    gfx.drawRoundRect(kPreviewX, kPreviewY, kPreviewW, kPreviewH, 8, kColorPanelEdge);

    gfx.setTextColor(TFT_CYAN, kColorCard);
    gfx.drawString("LED Preview", kPreviewX + 10, kPreviewY + 5, 1);
    gfx.setTextColor(TFT_WHITE, kColorCard);
    gfx.drawString("8 LEDs / GPIO14", kPreviewX + 90, kPreviewY + 5, 1);

    constexpr int16_t kCircleRadius = 10;
    const int16_t ledRowY = kPreviewY + 28;
    const int16_t startX = kPreviewX + 24;
    const int16_t gapX = 36;

    for (uint16_t index = 0; index < WS2812_NUM_LEDS; ++index) {
        const uint16_t fill = pixelTo565(strip.getPixelColor(index));
        const int16_t x = startX + static_cast<int16_t>(index) * gapX;
        gfx.fillCircle(x, ledRowY, kCircleRadius, fill);
        gfx.drawCircle(x, ledRowY, kCircleRadius, TFT_LIGHTGREY);
    }
}

template <typename Canvas>
void drawBrightnessPanel(Canvas &gfx)
{
    gfx.fillRoundRect(kBrightnessX, kBrightnessY, kBrightnessW, kBrightnessH, 8, kColorCard);
    gfx.drawRoundRect(kBrightnessX, kBrightnessY, kBrightnessW, kBrightnessH, 8, TFT_YELLOW);

    char brightnessText[40];
    snprintf(brightnessText,
             sizeof(brightnessText),
             "Brightness %u / 255  (%u%%)",
             currentBrightness(),
             currentBrightnessPercent());

    gfx.setTextColor(TFT_WHITE, kColorCard);
    gfx.drawString(brightnessText, kBrightnessX + 8, kBrightnessY + 4, 1);

    const int16_t barX = kBrightnessX + 170;
    const int16_t barY = kBrightnessY + 5;
    const int16_t barW = 126;
    const int16_t barH = 10;
    const int16_t fillW = static_cast<int16_t>((static_cast<uint32_t>(barW) * currentBrightness()) / 255U);

    gfx.fillRoundRect(barX, barY, barW, barH, 4, kColorBarBg);
    if (fillW > 0) {
        gfx.fillRoundRect(barX, barY, fillW, barH, 4, TFT_YELLOW);
    }
    gfx.drawRoundRect(barX, barY, barW, barH, 4, TFT_DARKGREY);
}

template <typename Canvas>
void drawFooter(Canvas &gfx)
{
    const int16_t y = gfx.height() - kFooterHeight;
    gfx.fillRect(0, y, gfx.width(), kFooterHeight, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString("USER: cycle brightness  |  Auto step: 2.5s", kUiMargin, y + 4, 1);
}

template <typename Canvas>
void drawUi(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kColorBg);
    drawHeader(gfx);
    drawStatusPanel(gfx);
    drawPreviewPanel(gfx);
    drawBrightnessPanel(gfx);
    drawFooter(gfx);
}
