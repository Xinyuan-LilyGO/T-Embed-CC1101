#pragma once

namespace page_ws2812 {

namespace {
struct PatternDef {
    const char* name;
    const char* detail;
    uint16_t accent;
    bool animated;
};

constexpr PatternDef kPatterns[] = {
    {"Solid Red",   "All red",         TFT_RED,    false},
    {"Solid Green", "All green",       TFT_GREEN,  false},
    {"Solid Blue",  "All blue",        TFT_BLUE,   false},
    {"White",       "RGB mixed white", TFT_WHITE,  false},
    {"Rainbow",     "Animated sweep",  TFT_CYAN,   true},
    {"Chase Cyan",  "Moving pixels",   TFT_YELLOW, true},
};
constexpr uint8_t kPatternCount = sizeof(kPatterns) / sizeof(kPatterns[0]);
constexpr uint8_t kBrightnessLevels[] = {8, 24, 48, 96, 160, 255};
constexpr uint8_t kBrightnessLevelCount = sizeof(kBrightnessLevels) / sizeof(kBrightnessLevels[0]);
constexpr uint32_t kPatternAdvanceMs = 2500;
constexpr uint32_t kAnimationIntervalMs = 100;
constexpr uint32_t kFrameMs = 80;

Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);
uint8_t currentPatternIndex = 0;
uint8_t currentBrightnessIndex = 1;
uint8_t animationPhase = 0;
bool backFocused = false;
bool ledDirty = true;
bool screenDirty = true;
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
                strip.setPixelColor(i, ((i + (animationPhase / 2)) % 3) == 0 ? strip.Color(0, 180, 255) : strip.Color(0, 0, 8));
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
}

void advancePattern()
{
    currentPatternIndex = static_cast<uint8_t>((currentPatternIndex + 1U) % kPatternCount);
    animationPhase = 0;
    lastPatternChangeMs = millis();
    ledDirty = true;
    screenDirty = true;
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
}  // namespace

void init()
{
    currentPatternIndex = 0;
    currentBrightnessIndex = 1;
    animationPhase = 0;
    backFocused = false;
    encSnapshot = g.encRaw;
    ledDirty = true;
    screenDirty = true;
    lastPatternChangeMs = millis();
    lastAnimationMs = millis();
    lastDrawMs = 0;
    strip.begin();
    strip.clear();
    strip.show();
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
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    tft.fillScreen(kUiBg);
    drawPageHeader("WS2812 LEDs", currentPattern().accent);
    drawPageFooter("USER=bright  BOOT=next", backFocused);
    drawCard(8, 34, 148, 42, "Pattern", currentPattern().name, currentPattern().accent);
    drawCard(164, 34, 148, 42, "Brightness", String(currentBrightness()) + "/255", TFT_YELLOW);

    for (uint8_t i = 0; i < WS2812_NUM_LEDS; ++i) {
        const uint16_t c = pixelTo565(strip.getPixelColor(i));
        tft.fillRoundRect(8 + i * 38, 96, 28, 28, 8, c);
        tft.drawRoundRect(8 + i * 38, 96, 28, 28, 8, TFT_WHITE);
    }

    tft.setTextColor(TFT_WHITE, kUiBg);
    tft.drawString(String(currentBrightnessPercent()) + "%  " + currentPattern().detail, 8, 132, 1);
    drawMeter(8, 142, 304, 8, currentBrightness(), 255, TFT_YELLOW);

    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    strip.clear();
    strip.show();
}

}  // namespace page_ws2812
