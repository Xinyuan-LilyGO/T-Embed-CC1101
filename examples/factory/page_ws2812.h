#pragma once

namespace page_ws2812 {

namespace {
struct PatternDef {
    const char* name;
    const char* detail;
    uint16_t accent;
    bool animated;
};

constexpr uint32_t kPatternAdvanceMs = 1500;
constexpr uint32_t kAnimationIntervalMs = 100;
constexpr uint8_t kBrightnessLevels[] = {8, 24, 48, 96, 160, 255};
constexpr uint8_t kBrightnessLevelCount = sizeof(kBrightnessLevels) / sizeof(kBrightnessLevels[0]);
constexpr uint32_t kFrameMs = 50;

constexpr int16_t kUiMargin = 8;
constexpr int16_t kStatusX = 8;
constexpr int16_t kStatusY = 34;
constexpr int16_t kStatusW = 304;
constexpr int16_t kStatusH = 38;
constexpr int16_t kPreviewX = 8;
constexpr int16_t kPreviewY = 80;
constexpr int16_t kPreviewW = 304;
constexpr int16_t kPreviewH = 42;
constexpr int16_t kBrightnessX = 8;
constexpr int16_t kBrightnessY = 128;
constexpr int16_t kBrightnessW = 304;
constexpr int16_t kBrightnessH = 20;

constexpr uint16_t kColorBg = 0x0841;
constexpr uint16_t kColorPanel = 0x1082;
constexpr uint16_t kColorPanelEdge = 0x31A6;
constexpr uint16_t kColorCard = 0x18C3;
constexpr uint16_t kColorBarBg = 0x2104;

constexpr PatternDef kPatterns[] = {
    {"Solid Red",   "Checks the red channel on all 8 LEDs.", TFT_RED,   false},
    {"Solid Green", "Checks the green channel on all 8 LEDs.", TFT_GREEN, false},
    {"Solid Blue",  "Checks the blue channel on all 8 LEDs.", TFT_BLUE,  false},
    {"White",       "Checks mixed RGB output on all LEDs.", TFT_WHITE, false},
    {"Rainbow",     "Animated hue sweep across the strip.", TFT_CYAN,  true},
    {"Chase Cyan",  "Moving pixels confirm animation timing.", TFT_YELLOW, true},
};
constexpr uint8_t kPatternCount = sizeof(kPatterns) / sizeof(kPatterns[0]);

Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

bool screenDirty = true;
bool ledDirty = true;
bool backFocused = false;
uint8_t currentPatternIndex = 0;
uint8_t currentBrightnessIndex = 1;
uint8_t animationPhase = 0;
uint32_t lastPatternChangeMs = 0;
uint32_t lastAnimationMs = 0;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;

uint8_t currentBrightness()
{
    return kBrightnessLevels[currentBrightnessIndex];
}

uint8_t currentBrightnessPercent()
{
    return static_cast<uint8_t>((static_cast<uint16_t>(currentBrightness()) * 100U) / 255U);
}

const PatternDef& currentPattern()
{
    return kPatterns[currentPatternIndex];
}

uint32_t wheelColor(uint8_t pos)
{
    pos = 255 - pos;
    if (pos < 85) {
        return strip.Color(255 - pos * 3, 0, pos * 3);
    }
    if (pos < 170) {
        pos -= 85;
        return strip.Color(0, pos * 3, 255 - pos * 3);
    }
    pos -= 170;
    return strip.Color(pos * 3, 255 - pos * 3, 0);
}

uint16_t pixelTo565(const uint32_t color)
{
    return tft.color565(static_cast<uint8_t>((color >> 16) & 0xFF),
                        static_cast<uint8_t>((color >> 8) & 0xFF),
                        static_cast<uint8_t>(color & 0xFF));
}

void renderCurrentPattern()
{
    strip.setBrightness(currentBrightness());
    strip.clear();

    switch (currentPatternIndex) {
        case 0:
            strip.fill(strip.Color(255, 0, 0));
            break;
        case 1:
            strip.fill(strip.Color(0, 255, 0));
            break;
        case 2:
            strip.fill(strip.Color(0, 0, 255));
            break;
        case 3:
            strip.fill(strip.Color(255, 255, 255));
            break;
        case 4:
            for (uint16_t i = 0; i < WS2812_NUM_LEDS; ++i) {
                const uint8_t wheel = static_cast<uint8_t>((i * 256 / WS2812_NUM_LEDS + animationPhase) & 0xFF);
                strip.setPixelColor(i, wheelColor(wheel));
            }
            break;
        case 5:
        default:
            for (uint16_t i = 0; i < WS2812_NUM_LEDS; ++i) {
                strip.setPixelColor(i, ((i + (animationPhase / 2)) % 3) == 0 ? strip.Color(0, 180, 255)
                                                                              : strip.Color(0, 0, 8));
            }
            break;
    }

    strip.show();
    ledDirty = false;
}

void advanceBrightness()
{
    currentBrightnessIndex = static_cast<uint8_t>((currentBrightnessIndex + 1U) % kBrightnessLevelCount);
    ledDirty = true;
    screenDirty = true;
    Serial.printf("[WS2812] Brightness -> %u / 255 (%u%%)\n", currentBrightness(), currentBrightnessPercent());
}

void advancePattern()
{
    currentPatternIndex = static_cast<uint8_t>((currentPatternIndex + 1U) % kPatternCount);
    animationPhase = 0;
    lastPatternChangeMs = millis();
    ledDirty = true;
    screenDirty = true;
    Serial.printf("[WS2812] Pattern -> %s\n", currentPattern().name);
}

void handlePatternTimers()
{
    const uint32_t now = millis();
    if ((now - lastPatternChangeMs) >= kPatternAdvanceMs) {
        advancePattern();
    }
    if (currentPattern().animated && (now - lastAnimationMs) >= kAnimationIntervalMs) {
        lastAnimationMs = now;
        animationPhase = static_cast<uint8_t>(animationPhase + 7U);
        ledDirty = true;
        screenDirty = true;
    }
}

void drawHeader()
{
    tft.fillRect(0, 0, tft.width(), kHeaderH, TFT_NAVY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("WS2812 Test", kUiMargin, 5, 2);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.drawString("NeoPixel", tft.width() - kUiMargin, 7, 1);
    tft.setTextDatum(TL_DATUM);
}

void drawStatusPanel()
{
    const PatternDef& pattern = currentPattern();
    tft.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, kColorPanel);
    tft.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, pattern.accent);

    tft.setTextColor(pattern.accent, kColorPanel);
    tft.drawString(pattern.name, kStatusX + 10, kStatusY + 6, 2);

    char stepLabel[16];
    snprintf(stepLabel, sizeof(stepLabel), "%u/%u", currentPatternIndex + 1, kPatternCount);
    tft.fillRoundRect(kStatusX + kStatusW - 52, kStatusY + 7, 40, 16, 6, pattern.accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, pattern.accent);
    tft.drawString(stepLabel, kStatusX + kStatusW - 32, kStatusY + 15, 1);
    tft.setTextDatum(TL_DATUM);

    tft.setTextColor(TFT_LIGHTGREY, kColorPanel);
    tft.drawString(pattern.detail, kStatusX + 10, kStatusY + 24, 1);
}

void drawPreviewPanel()
{
    tft.fillRoundRect(kPreviewX, kPreviewY, kPreviewW, kPreviewH, 8, kColorCard);
    tft.drawRoundRect(kPreviewX, kPreviewY, kPreviewW, kPreviewH, 8, kColorPanelEdge);

    tft.setTextColor(TFT_CYAN, kColorCard);
    tft.drawString("LED Preview", kPreviewX + 10, kPreviewY + 5, 1);
    tft.setTextColor(TFT_WHITE, kColorCard);
    tft.drawString("8 LEDs / GPIO14", kPreviewX + 90, kPreviewY + 5, 1);

    constexpr int16_t kCircleRadius = 10;
    const int16_t ledRowY = kPreviewY + 28;
    const int16_t startX = kPreviewX + 24;
    const int16_t gapX = 36;

    for (uint16_t i = 0; i < WS2812_NUM_LEDS; ++i) {
        const uint16_t fill = pixelTo565(strip.getPixelColor(i));
        const int16_t x = startX + static_cast<int16_t>(i) * gapX;
        tft.fillCircle(x, ledRowY, kCircleRadius, fill);
        tft.drawCircle(x, ledRowY, kCircleRadius, TFT_LIGHTGREY);
    }
}

void drawBrightnessPanel()
{
    tft.fillRoundRect(kBrightnessX, kBrightnessY, kBrightnessW, kBrightnessH, 8, kColorCard);
    tft.drawRoundRect(kBrightnessX, kBrightnessY, kBrightnessW, kBrightnessH, 8, TFT_YELLOW);

    char brightnessText[40];
    snprintf(brightnessText, sizeof(brightnessText), "Brightness %u / 255  (%u%%)",
             currentBrightness(), currentBrightnessPercent());
    tft.setTextColor(TFT_WHITE, kColorCard);
    tft.drawString(brightnessText, kBrightnessX + 8, kBrightnessY + 4, 1);

    const int16_t barX = kBrightnessX + 170;
    const int16_t barY = kBrightnessY + 5;
    const int16_t barW = 126;
    const int16_t barH = 10;
    const int16_t fillW = static_cast<int16_t>((static_cast<uint32_t>(barW) * currentBrightness()) / 255U);

    tft.fillRoundRect(barX, barY, barW, barH, 4, kColorBarBg);
    if (fillW > 0) {
        tft.fillRoundRect(barX, barY, fillW, barH, 4, TFT_YELLOW);
    }
    tft.drawRoundRect(barX, barY, barW, barH, 4, TFT_DARKGREY);
}

void drawFooter()
{
    const int16_t y = tft.height() - kFooterH;
    tft.fillRect(0, y, tft.width(), kFooterH, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString(backFocused ? "BOOT back" : "USER bright  BOOT next", kUiMargin, y + 4, 1);
    drawBackButton(backFocused);
}

void redrawAll()
{
    board_prepare_display();
    tft.fillRect(0, 0, tft.width(), tft.height(), kColorBg);
    drawHeader();
    drawStatusPanel();
    drawPreviewPanel();
    drawBrightnessPanel();
    drawFooter();
    board_spi_deselect_all();
}
}  // namespace

void init()
{
    screenDirty = true;
    ledDirty = true;
    backFocused = false;
    currentPatternIndex = 0;
    currentBrightnessIndex = 1;
    animationPhase = 0;
    lastPatternChangeMs = millis();
    lastAnimationMs = millis();
    lastDrawMs = 0;
    encSnapshot = g.encRaw;

    strip.begin();
    strip.clear();
    strip.show();
    renderCurrentPattern();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    if (takeUserButton() && !backFocused) {
        advanceBrightness();
    }

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        advancePattern();
    }

    handlePatternTimers();
    if (ledDirty) {
        renderCurrentPattern();
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0U && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    gSubPageGfx.beginFrame();
    redrawAll();
    gSubPageGfx.endFrame();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    strip.clear();
    strip.show();
}

}  // namespace page_ws2812
