#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>

#include "utilities.h"

namespace {

struct PatternDef {
    const char *name;
    const char *detail;
    uint16_t accent;
    bool animated;
};

constexpr uint8_t kRotation = 3;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr uint32_t kPatternAdvanceMs = 2500;
constexpr uint32_t kAnimationIntervalMs = 100;
constexpr uint8_t kBrightnessLevels[] = {8, 24, 48, 96, 160, 255};

constexpr int16_t kUiMargin = 8;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;
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
    {"Solid Red", "Checks the red channel on all 8 LEDs.", TFT_RED, false},
    {"Solid Green", "Checks the green channel on all 8 LEDs.", TFT_GREEN, false},
    {"Solid Blue", "Checks the blue channel on all 8 LEDs.", TFT_BLUE, false},
    {"White", "Checks mixed RGB output on all LEDs.", TFT_WHITE, false},
    {"Rainbow", "Animated hue sweep across the strip.", TFT_CYAN, true},
    {"Chase Cyan", "Moving pixels confirm animation timing.", TFT_YELLOW, true},
};

constexpr uint8_t kPatternCount = sizeof(kPatterns) / sizeof(kPatterns[0]);
constexpr uint8_t kBrightnessLevelCount = sizeof(kBrightnessLevels) / sizeof(kBrightnessLevels[0]);

TFT_eSPI tft;
TFT_eSprite canvas(&tft);
Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

bool canvasReady = false;
bool screenDirty = true;
bool ledDirty = true;

uint8_t currentPatternIndex = 0;
uint8_t currentBrightnessIndex = 1;
uint8_t animationPhase = 0;

uint32_t lastPatternChangeMs = 0;
uint32_t lastAnimationMs = 0;

void deselectSharedSpiDevices()
{
    pinMode(DISPLAY_CS, OUTPUT);
    digitalWrite(DISPLAY_CS, HIGH);

    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);

    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

    pinMode(BOARD_NRF24_CS, OUTPUT);
    digitalWrite(BOARD_NRF24_CS, HIGH);
}

bool initBoardForDisplay()
{
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);

    deselectSharedSpiDevices();

    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    tft.begin();
    tft.setRotation(kRotation);
    tft.fillScreen(TFT_BLACK);

    digitalWrite(DISPLAY_BL, HIGH);
    return true;
}

bool boardUserKeyPressed()
{
    static bool lastRawState = HIGH;
    static bool stableState = HIGH;
    static uint32_t lastChangeMs = 0;

    const bool rawState = digitalRead(BOARD_USER_KEY);
    const uint32_t now = millis();

    if (rawState != lastRawState) {
        lastRawState = rawState;
        lastChangeMs = now;
    }

    if ((now - lastChangeMs) >= kButtonDebounceMs && stableState != rawState) {
        stableState = rawState;
        if (stableState == LOW) {
            return true;
        }
    }

    return false;
}

uint8_t currentBrightness()
{
    return kBrightnessLevels[currentBrightnessIndex];
}

uint8_t currentBrightnessPercent()
{
    return static_cast<uint8_t>((static_cast<uint16_t>(currentBrightness()) * 100U) / 255U);
}

const PatternDef &currentPattern()
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

uint16_t pixelTo565(uint32_t color)
{
    const uint8_t red = static_cast<uint8_t>((color >> 16) & 0xFF);
    const uint8_t green = static_cast<uint8_t>((color >> 8) & 0xFF);
    const uint8_t blue = static_cast<uint8_t>(color & 0xFF);
    return tft.color565(red, green, blue);
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
        for (uint16_t index = 0; index < WS2812_NUM_LEDS; ++index) {
            const uint8_t wheel = static_cast<uint8_t>((index * 256 / WS2812_NUM_LEDS + animationPhase) & 0xFF);
            strip.setPixelColor(index, wheelColor(wheel));
        }
        break;
    case 5:
    default:
        for (uint16_t index = 0; index < WS2812_NUM_LEDS; ++index) {
            if (((index + (animationPhase / 2)) % 3) == 0) {
                strip.setPixelColor(index, strip.Color(0, 180, 255));
            } else {
                strip.setPixelColor(index, strip.Color(0, 0, 8));
            }
        }
        break;
    }

    strip.show();
    ledDirty = false;
}

#include "test_ws2812_ui.h"

void redrawScreen()
{
    deselectSharedSpiDevices();

    if (canvasReady) {
        drawUi(canvas);
        canvas.pushSprite(0, 0);
    } else {
        drawUi(tft);
    }

    deselectSharedSpiDevices();
    screenDirty = false;
}

void advanceBrightness()
{
    currentBrightnessIndex = static_cast<uint8_t>((currentBrightnessIndex + 1U) % kBrightnessLevelCount);
    ledDirty = true;
    screenDirty = true;

    Serial.printf("[WS2812] Brightness -> %u / 255 (%u%%)\n",
                  currentBrightness(),
                  currentBrightnessPercent());
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

void initStrip()
{
    strip.begin();
    strip.clear();
    strip.show();
    ledDirty = true;
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("T-Embed WS2812 test"));

    pinMode(BOARD_USER_KEY, INPUT_PULLUP);

    if (!initBoardForDisplay()) {
        Serial.println(F("[WS2812] Display init failed. Halting."));
        while (true) {
            delay(1000);
        }
    }

    canvas.setColorDepth(16);
    canvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!canvasReady) {
        Serial.println(F("[WS2812] Sprite allocation failed, using direct TFT redraw."));
    }

    initStrip();
    lastPatternChangeMs = millis();
    lastAnimationMs = millis();

    renderCurrentPattern();
    redrawScreen();
}

void loop()
{
    if (boardUserKeyPressed()) {
        advanceBrightness();
    }

    handlePatternTimers();

    if (ledDirty) {
        renderCurrentPattern();
    }

    if (screenDirty) {
        redrawScreen();
    }

    delay(10);
}
