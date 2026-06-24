#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PN532.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <RadioLib.h>
#include <driver/i2s.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <math.h>

#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include <bq27220.h>

#include "utilities.h"

void requestExitSubPage();
void requestSystemSleep();
String currentRotationLabel();
String autoSleepPresetLabel();
String autoDimTimeoutLabel();
void cycleDisplayRotation();
void cycleAutoSleepPreset();

namespace page_battery { void init(); void update(); void render(); void deinit(); }
namespace page_cc1101  { void init(); void update(); void render(); void deinit(); }
namespace page_ir      { void init(); void update(); void render(); void deinit(); }
namespace page_mic     { void init(); void update(); void render(); void deinit(); }
namespace page_nfc     { void init(); void update(); void render(); void deinit(); }
namespace page_nrf24   { void init(); void update(); void render(); void deinit(); }
namespace page_sd      { void init(); void update(); void render(); void deinit(); }
namespace page_wifi    { void init(); void update(); void render(); void deinit(); }
namespace page_tft     { void init(); void update(); void render(); void deinit(); }
namespace page_ws2812  { void init(); void update(); void render(); void deinit(); }
namespace page_setting { void init(); void update(); void render(); void deinit(); }

enum class PageId : uint8_t {
    MainMenu = 0,
    Battery,
    CC1101,
    IR,
    Mic,
    NFC,
    Nrf24,
    SD,
    WiFi,
    TFT,
    WS2812,
    Setting,
    kCount,
};

constexpr uint8_t kPageCount = static_cast<uint8_t>(PageId::kCount) - 1;

struct PageDescriptor {
    const char* label;
    void (*init)();
    void (*update)();
    void (*render)();
    void (*deinit)();
};

enum class AutoSleepPreset : uint8_t {
    Off = 0,
    Sec30,
    Min1,
    Min2,
    Min5,
};

struct FactorySettings {
    uint8_t rotation = 3;
    AutoSleepPreset autoSleepPreset = AutoSleepPreset::Off;
};

struct Btn {
    uint8_t pin = 0;
    bool pressed = false;
    bool event = false;
    uint32_t lastChangeMs = 0;
};

struct FactoryState {
    PageId activePage = PageId::MainMenu;
    int8_t menuCursor = 0;
    bool menuDirty = true;
    bool subPageExitRequested = false;
    bool systemSleepRequested = false;
    bool backlightDimmed = false;
    volatile int32_t encRaw = 0;
    int32_t encLast = 0;
    int32_t encActivitySnapshot = 0;
    volatile uint8_t encPrevAB = 0;
    uint32_t lastUserInputMs = 0;
    Btn encBtn;
    Btn usrBtn;
} g;

TFT_eSPI tft;
TFT_eSprite gMainMenuCanvas(&tft);
class SubPageGfxProxy {
public:
    explicit SubPageGfxProxy(TFT_eSPI* display)
        : display_(display), sprite_(display) {}

    void beginFrame()
    {
        active_ = true;
        if (ensureSprite()) {
            usingSprite_ = true;
        } else {
            usingSprite_ = false;
        }
    }

    void endFrame()
    {
        if (active_ && usingSprite_) {
            sprite_.pushSprite(0, 0);
        }
        active_ = false;
        usingSprite_ = false;
    }

    void invalidate()
    {
        sprite_.deleteSprite();
        spriteReady_ = false;
        spriteW_ = 0;
        spriteH_ = 0;
        active_ = false;
        usingSprite_ = false;
    }

    int16_t width() { return usingSprite_ ? sprite_.width() : display_->width(); }
    int16_t height() { return usingSprite_ ? sprite_.height() : display_->height(); }
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return display_->color565(r, g, b); }
    SPIClass& getSPIinstance() { return display_->getSPIinstance(); }

    void fillScreen(uint32_t color) { if (usingSprite_) sprite_.fillScreen(color); else display_->fillScreen(color); }
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) { if (usingSprite_) sprite_.fillRect(x, y, w, h, color); else display_->fillRect(x, y, w, h, color); }
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) { if (usingSprite_) sprite_.drawRect(x, y, w, h, color); else display_->drawRect(x, y, w, h, color); }
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) { if (usingSprite_) sprite_.fillRoundRect(x, y, w, h, r, color); else display_->fillRoundRect(x, y, w, h, r, color); }
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color) { if (usingSprite_) sprite_.drawRoundRect(x, y, w, h, r, color); else display_->drawRoundRect(x, y, w, h, r, color); }
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) { if (usingSprite_) sprite_.drawFastHLine(x, y, w, color); else display_->drawFastHLine(x, y, w, color); }
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) { if (usingSprite_) sprite_.drawFastVLine(x, y, h, color); else display_->drawFastVLine(x, y, h, color); }
    void fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color) { if (usingSprite_) sprite_.fillCircle(x, y, r, color); else display_->fillCircle(x, y, r, color); }
    void drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color) { if (usingSprite_) sprite_.drawCircle(x, y, r, color); else display_->drawCircle(x, y, r, color); }
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) { if (usingSprite_) sprite_.drawLine(x0, y0, x1, y1, color); else display_->drawLine(x0, y0, x1, y1, color); }

    void setTextDatum(uint8_t datum) { if (usingSprite_) sprite_.setTextDatum(datum); else display_->setTextDatum(datum); }
    void setTextPadding(uint16_t padding) { if (usingSprite_) sprite_.setTextPadding(padding); else display_->setTextPadding(padding); }
    void setTextColor(uint16_t fg) { if (usingSprite_) sprite_.setTextColor(fg); else display_->setTextColor(fg); }
    void setTextColor(uint16_t fg, uint16_t bg) { if (usingSprite_) sprite_.setTextColor(fg, bg); else display_->setTextColor(fg, bg); }

    int16_t drawString(const String& text, int32_t x, int32_t y, uint8_t font = 1) { return usingSprite_ ? sprite_.drawString(text, x, y, font) : display_->drawString(text, x, y, font); }
    int16_t drawString(const char* text, int32_t x, int32_t y, uint8_t font = 1) { return usingSprite_ ? sprite_.drawString(text, x, y, font) : display_->drawString(text, x, y, font); }
    int16_t drawCentreString(const String& text, int32_t x, int32_t y, uint8_t font = 1) { return usingSprite_ ? sprite_.drawCentreString(text, x, y, font) : display_->drawCentreString(text, x, y, font); }
    int16_t drawCentreString(const char* text, int32_t x, int32_t y, uint8_t font = 1) { return usingSprite_ ? sprite_.drawCentreString(text, x, y, font) : display_->drawCentreString(text, x, y, font); }
    int16_t drawRightString(const String& text, int32_t x, int32_t y, uint8_t font = 1) { return usingSprite_ ? sprite_.drawRightString(text, x, y, font) : display_->drawRightString(text, x, y, font); }
    int16_t drawRightString(const char* text, int32_t x, int32_t y, uint8_t font = 1) { return usingSprite_ ? sprite_.drawRightString(text, x, y, font) : display_->drawRightString(text, x, y, font); }

private:
    bool ensureSprite()
    {
        const int16_t w = display_->width();
        const int16_t h = display_->height();
        if (spriteReady_ && spriteW_ == w && spriteH_ == h) {
            return true;
        }

        sprite_.deleteSprite();
        sprite_.setColorDepth(16);
        spriteReady_ = (sprite_.createSprite(w, h) != nullptr);
        if (spriteReady_) {
            spriteW_ = w;
            spriteH_ = h;
        } else {
            spriteW_ = 0;
            spriteH_ = 0;
            Serial.println(F("[MAIN] Sub-page sprite allocation failed, using direct TFT redraw."));
        }
        return spriteReady_;
    }

    TFT_eSPI* display_;
    TFT_eSprite sprite_;
    bool active_ = false;
    bool usingSprite_ = false;
    bool spriteReady_ = false;
    int16_t spriteW_ = 0;
    int16_t spriteH_ = 0;
} gSubPageGfx(&tft);
Preferences gPrefs;
FactorySettings gSettings;
bool gPrefsReady = false;
bool gMainMenuCanvasReady = false;
uint32_t gMainMenuLastDrawMs = 0;

static const PageDescriptor kPages[] = {
    { "Battery / PMU", page_battery::init, page_battery::update, page_battery::render, page_battery::deinit },
    { "CC1101 Radio",  page_cc1101::init,  page_cc1101::update,  page_cc1101::render,  page_cc1101::deinit  },
    { "IR TX / RX",    page_ir::init,      page_ir::update,      page_ir::render,      page_ir::deinit      },
    { "MIC & Speaker", page_mic::init,     page_mic::update,     page_mic::render,     page_mic::deinit     },
    { "PN532 NFC",     page_nfc::init,     page_nfc::update,     page_nfc::render,     page_nfc::deinit     },
    { "nRF24 Tx/Rx",   page_nrf24::init,   page_nrf24::update,   page_nrf24::render,   page_nrf24::deinit   },
    { "SD Card",       page_sd::init,      page_sd::update,      page_sd::render,      page_sd::deinit      },
    { "WiFi",          page_wifi::init,    page_wifi::update,    page_wifi::render,    page_wifi::deinit    },
    { "TFT Display",   page_tft::init,     page_tft::update,     page_tft::render,     page_tft::deinit     },
    { "WS2812 LEDs",   page_ws2812::init,  page_ws2812::update,  page_ws2812::render,  page_ws2812::deinit  },
    { "Settings",      page_setting::init, page_setting::update, page_setting::render, page_setting::deinit },
};

constexpr uint8_t  kEncA = ENCODER_INA;
constexpr uint8_t  kEncB = ENCODER_INB;
constexpr uint32_t kDebounceMs = 20;
constexpr uint8_t  kRotationLandscape = 3;
constexpr uint8_t  kRotationReverseLandscape = 1;
constexpr uint32_t kMinDimLeadMs = 10000;
constexpr char kPrefsNamespace[] = "factory";
constexpr char kPrefRotationKey[] = "rotation";
constexpr char kPrefAutoSleepKey[] = "autosleep";

static const int8_t kEncTable[4][4] = {
    { 0, -1,  1,  0},
    { 1,  0,  0, -1},
    {-1,  0,  0,  1},
    { 0,  1, -1,  0},
};

constexpr uint16_t kUiBg        = 0x0841;
constexpr uint16_t kUiPanel     = 0x1082;
constexpr uint16_t kUiCard      = 0x18C3;
constexpr uint16_t kUiEdge      = 0x31A6;
constexpr uint16_t kUiMuted     = 0x9CD3;
constexpr uint16_t kPassBg      = 0x0A41;
constexpr uint16_t kFailBg      = 0x3006;
constexpr int16_t  kHeaderH     = 24;
constexpr int16_t  kFooterH     = 18;
constexpr int16_t  kMargin      = 8;

void board_spi_deselect_all()
{
    pinMode(DISPLAY_CS, OUTPUT);
    pinMode(BOARD_SD_CS, OUTPUT);
    pinMode(BOARD_LORA_CS, OUTPUT);
    pinMode(BOARD_NRF24_CS, OUTPUT);
    digitalWrite(DISPLAY_CS, HIGH);
    digitalWrite(BOARD_SD_CS, HIGH);
    digitalWrite(BOARD_LORA_CS, HIGH);
    digitalWrite(BOARD_NRF24_CS, HIGH);

    pinMode(BOARD_NRF24_CE, OUTPUT);
    digitalWrite(BOARD_NRF24_CE, LOW);
}

void board_spi_init_shared_bus()
{
    board_spi_deselect_all();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
}

void board_prepare_display()
{
    board_spi_deselect_all();
}

bool i2cDevicePresent(const uint8_t addr)
{
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

uint8_t normalizeRotation(const uint8_t rotation)
{
    return rotation == kRotationReverseLandscape ? kRotationReverseLandscape : kRotationLandscape;
}

AutoSleepPreset normalizeAutoSleepPreset(const uint8_t raw)
{
    switch (raw) {
        case static_cast<uint8_t>(AutoSleepPreset::Sec30): return AutoSleepPreset::Sec30;
        case static_cast<uint8_t>(AutoSleepPreset::Min1):  return AutoSleepPreset::Min1;
        case static_cast<uint8_t>(AutoSleepPreset::Min2):  return AutoSleepPreset::Min2;
        case static_cast<uint8_t>(AutoSleepPreset::Min5):  return AutoSleepPreset::Min5;
        case static_cast<uint8_t>(AutoSleepPreset::Off):
        default: return AutoSleepPreset::Off;
    }
}

void loadFactorySettings()
{
    if (!gPrefs.begin(kPrefsNamespace, false)) {
        gPrefsReady = false;
        gSettings = FactorySettings{};
        return;
    }

    gPrefsReady = true;
    gSettings.rotation = normalizeRotation(gPrefs.getUChar(kPrefRotationKey, kRotationLandscape));
    gSettings.autoSleepPreset = normalizeAutoSleepPreset(
        gPrefs.getUChar(kPrefAutoSleepKey, static_cast<uint8_t>(AutoSleepPreset::Off)));
}

void saveRotationSetting()
{
    if (gPrefsReady) {
        gPrefs.putUChar(kPrefRotationKey, gSettings.rotation);
    }
}

void saveAutoSleepSetting()
{
    if (gPrefsReady) {
        gPrefs.putUChar(kPrefAutoSleepKey, static_cast<uint8_t>(gSettings.autoSleepPreset));
    }
}

void setBacklightBrightness(const uint8_t value)
{
    digitalWrite(DISPLAY_BL, value == 0 ? LOW : HIGH);
}

void noteUserActivity()
{
    g.lastUserInputMs = millis();
    if (g.backlightDimmed) {
        g.backlightDimmed = false;
        setBacklightBrightness(255);
    }
}

String formatDurationLabel(const uint32_t ms)
{
    if (ms == 0) {
        return "Off";
    }
    const uint32_t sec = (ms + 500U) / 1000U;
    if (sec < 60U) {
        return String(sec) + "s";
    }
    if ((sec % 60U) == 0U) {
        return String(sec / 60U) + "m";
    }
    return String(sec / 60U) + "m " + String(sec % 60U) + "s";
}

uint32_t autoSleepTimeoutMs()
{
    switch (gSettings.autoSleepPreset) {
        case AutoSleepPreset::Sec30: return 30000UL;
        case AutoSleepPreset::Min1:  return 60000UL;
        case AutoSleepPreset::Min2:  return 120000UL;
        case AutoSleepPreset::Min5:  return 300000UL;
        case AutoSleepPreset::Off:
        default: return 0;
    }
}

uint32_t autoDimTimeoutMs()
{
    const uint32_t sleepMs = autoSleepTimeoutMs();
    if (sleepMs == 0) {
        return 0;
    }

    uint32_t dimMs = (sleepMs * 2UL) / 3UL;
    if ((sleepMs - dimMs) < kMinDimLeadMs) {
        dimMs = sleepMs > kMinDimLeadMs ? sleepMs - kMinDimLeadMs : sleepMs / 2UL;
    }
    return dimMs;
}

String currentRotationLabel()
{
    return gSettings.rotation == kRotationLandscape ? "Landscape" : "Reverse";
}

String autoSleepPresetLabel()
{
    return formatDurationLabel(autoSleepTimeoutMs());
}

String autoDimTimeoutLabel()
{
    return formatDurationLabel(autoDimTimeoutMs());
}

void releaseMainMenuCanvas()
{
    gMainMenuCanvas.deleteSprite();
    gMainMenuCanvasReady = false;
}

void initMainMenuCanvas()
{
    releaseMainMenuCanvas();
    gMainMenuCanvas.setColorDepth(16);
    gMainMenuCanvasReady = (gMainMenuCanvas.createSprite(tft.width(), tft.height()) != nullptr);
    gMainMenuLastDrawMs = 0;
    if (!gMainMenuCanvasReady) {
        Serial.println(F("[MAIN] Menu sprite allocation failed, using direct TFT redraw."));
    }
}

void setDisplayRotation(const uint8_t rotation)
{
    const uint8_t next = normalizeRotation(rotation);
    if (gSettings.rotation == next) {
        return;
    }

    gSettings.rotation = next;
    saveRotationSetting();
    tft.setRotation(gSettings.rotation);
    tft.fillScreen(TFT_BLACK);
    board_spi_deselect_all();
    gSubPageGfx.invalidate();

    if (g.activePage == PageId::MainMenu) {
        initMainMenuCanvas();
        g.menuDirty = true;
    }
}

void cycleDisplayRotation()
{
    setDisplayRotation(gSettings.rotation == kRotationLandscape ? kRotationReverseLandscape : kRotationLandscape);
}

void cycleAutoSleepPreset()
{
    switch (gSettings.autoSleepPreset) {
        case AutoSleepPreset::Off:   gSettings.autoSleepPreset = AutoSleepPreset::Sec30; break;
        case AutoSleepPreset::Sec30: gSettings.autoSleepPreset = AutoSleepPreset::Min1;  break;
        case AutoSleepPreset::Min1:  gSettings.autoSleepPreset = AutoSleepPreset::Min2;  break;
        case AutoSleepPreset::Min2:  gSettings.autoSleepPreset = AutoSleepPreset::Min5;  break;
        case AutoSleepPreset::Min5:
        default:                     gSettings.autoSleepPreset = AutoSleepPreset::Off;   break;
    }
    saveAutoSleepSetting();
    noteUserActivity();
}

void requestExitSubPage()
{
    g.subPageExitRequested = true;
}

void requestSystemSleep()
{
    g.systemSleepRequested = true;
}

void drawBackButton(const bool selected)
{
    const int16_t w = 58;
    const int16_t h = 14;
    const int16_t x = tft.width() - w - 6;
    const int16_t y = tft.height() - kFooterH + 2;
    const uint16_t bg = selected ? TFT_WHITE : TFT_DARKGREY;
    const uint16_t fg = selected ? TFT_BLACK : TFT_LIGHTGREY;

    gSubPageGfx.fillRoundRect(x, y, w, h, 5, bg);
    gSubPageGfx.drawRoundRect(x, y, w, h, 5, selected ? TFT_YELLOW : 0x52AA);
    gSubPageGfx.setTextColor(fg, bg);
    gSubPageGfx.drawCentreString("BACK", x + w / 2, y + 3, 1);
}

void drawPageHeader(const char* title, const uint16_t accent)
{
    gSubPageGfx.fillRect(0, 0, gSubPageGfx.width(), kHeaderH, TFT_NAVY);
    gSubPageGfx.setTextDatum(TL_DATUM);
    gSubPageGfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gSubPageGfx.drawString(title, kMargin, 5, 2);
    gSubPageGfx.setTextDatum(TR_DATUM);
    gSubPageGfx.setTextColor(accent, TFT_NAVY);
    gSubPageGfx.drawString("Factory", gSubPageGfx.width() - kMargin, 7, 1);
    gSubPageGfx.setTextDatum(TL_DATUM);
}

void drawPageFooter(const char* text, const bool backSelected = false)
{
    const int16_t y = gSubPageGfx.height() - kFooterH;
    gSubPageGfx.fillRect(0, y, gSubPageGfx.width(), kFooterH, TFT_DARKGREY);
    gSubPageGfx.setTextDatum(TL_DATUM);
    gSubPageGfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gSubPageGfx.drawString(text, kMargin, y + 4, 1);
    drawBackButton(backSelected);
}

void drawCard(const int16_t x, const int16_t y, const int16_t w, const int16_t h,
              const char* label, const String& value, const uint16_t accent)
{
    gSubPageGfx.fillRoundRect(x, y, w, h, 6, kUiCard);
    gSubPageGfx.drawRoundRect(x, y, w, h, 6, kUiEdge);
    gSubPageGfx.setTextDatum(TL_DATUM);
    gSubPageGfx.setTextColor(kUiMuted, kUiCard);
    gSubPageGfx.drawString(label, x + 8, y + 5, 1);
    gSubPageGfx.setTextColor(accent, kUiCard);
    gSubPageGfx.drawString(value, x + 8, y + 18, 2);
}

void drawStatusPill(const char* label, const bool ok, const int16_t x, const int16_t y)
{
    const uint16_t bg = ok ? kPassBg : kFailBg;
    const uint16_t fg = ok ? TFT_GREEN : TFT_RED;
    gSubPageGfx.fillRoundRect(x, y, 72, 20, 6, bg);
    gSubPageGfx.drawRoundRect(x, y, 72, 20, 6, ok ? TFT_DARKGREEN : TFT_MAROON);
    gSubPageGfx.setTextColor(fg, bg);
    gSubPageGfx.drawCentreString(label, x + 36, y + 5, 1);
}

void drawMeter(const int16_t x, const int16_t y, const int16_t w, const int16_t h,
               const uint16_t value, const uint16_t maxValue, const uint16_t color)
{
    const uint16_t clipped = min<uint16_t>(value, maxValue);
    const int16_t fillW = maxValue == 0 ? 0 : static_cast<int16_t>((static_cast<uint32_t>(w - 4) * clipped) / maxValue);
    gSubPageGfx.fillRoundRect(x, y, w, h, 5, 0x2104);
    gSubPageGfx.drawRoundRect(x, y, w, h, 5, kUiEdge);
    gSubPageGfx.fillRoundRect(x + 2, y + 2, fillW, h - 4, 4, color);
}

int32_t takeEncoderDelta(int32_t& snapshot)
{
    const int32_t delta = (g.encRaw - snapshot) / 2;
    if (delta != 0) {
        snapshot += delta * 2;
    }
    return delta;
}

bool takeEncoderButton()
{
    if (!g.encBtn.event) {
        return false;
    }
    g.encBtn.event = false;
    return true;
}

bool takeUserButton()
{
    if (!g.usrBtn.event) {
        return false;
    }
    g.usrBtn.event = false;
    return true;
}

bool updateBinaryBackFocus(int32_t& snapshot, bool& backFocused)
{
    const int32_t delta = takeEncoderDelta(snapshot);
    if (delta > 0 && !backFocused) {
        backFocused = true;
        return true;
    }
    if (delta < 0 && backFocused) {
        backFocused = false;
        return true;
    }
    return false;
}

void IRAM_ATTR onEncoderChange()
{
    const uint8_t a = digitalRead(kEncA);
    const uint8_t b = digitalRead(kEncB);
    const uint8_t cur = static_cast<uint8_t>((a << 1) | b);
    g.encRaw += kEncTable[g.encPrevAB][cur];
    g.encPrevAB = cur;
}

void pollButton(Btn& btn)
{
    const bool raw = (digitalRead(btn.pin) == LOW);
    const uint32_t now = millis();
    if (raw != btn.pressed && (now - btn.lastChangeMs) >= kDebounceMs) {
        btn.lastChangeMs = now;
        btn.pressed = raw;
        if (raw) {
            btn.event = true;
        }
    }
}

void trackUserActivity()
{
    if (g.encRaw != g.encActivitySnapshot) {
        g.encActivitySnapshot = g.encRaw;
        noteUserActivity();
    }
    if (g.encBtn.event || g.usrBtn.event) {
        noteUserActivity();
    }
}

void handleIdlePowerState()
{
    const uint32_t sleepMs = autoSleepTimeoutMs();
    if (sleepMs == 0) {
        return;
    }

    const uint32_t idleMs = millis() - g.lastUserInputMs;
    const uint32_t dimMs = autoDimTimeoutMs();
    if (!g.backlightDimmed && dimMs > 0 && idleMs >= dimMs) {
        g.backlightDimmed = true;
        setBacklightBrightness(1);
    }
    if (idleMs >= sleepMs) {
        requestSystemSleep();
    }
}

#include "main_menu_ui.h"

void renderMenu()
{
    const uint32_t now = millis();
    if (gMainMenuLastDrawMs != 0 && (now - gMainMenuLastDrawMs) < kMenuFrameIntervalMs) {
        return;
    }

    board_prepare_display();
    if (gMainMenuCanvasReady) {
        drawMenuUi(gMainMenuCanvas);
        gMainMenuCanvas.pushSprite(0, 0);
    } else {
        drawMenuUi(tft);
    }
    board_spi_deselect_all();

    g.menuDirty = false;
    gMainMenuLastDrawMs = now;
}

void enterSubPage(const PageId id)
{
    const uint8_t idx = static_cast<uint8_t>(id) - 1;
    if (idx >= kPageCount) {
        return;
    }

    g.activePage = id;
    g.subPageExitRequested = false;
    g.encLast = g.encRaw;
    noteUserActivity();
    releaseMainMenuCanvas();
    tft.fillScreen(TFT_BLACK);
    board_spi_deselect_all();
    kPages[idx].init();
    if (!g.subPageExitRequested) {
        kPages[idx].render();
    }
}

void exitSubPage()
{
    const uint8_t idx = static_cast<uint8_t>(g.activePage) - 1;
    if (idx < kPageCount) {
        kPages[idx].deinit();
    }
    g.activePage = PageId::MainMenu;
    g.subPageExitRequested = false;
    g.menuDirty = true;
    g.encLast = g.encRaw;
    noteUserActivity();
    initMainMenuCanvas();
    renderMenu();
}

void enterSystemSleepNow()
{
    Serial.println(F("[MAIN] Entering deep sleep. Wake with USER key."));
    g.systemSleepRequested = false;

    if (g.activePage != PageId::MainMenu) {
        const uint8_t idx = static_cast<uint8_t>(g.activePage) - 1;
        if (idx < kPageCount) {
            kPages[idx].deinit();
        }
    }

    board_spi_deselect_all();
    setBacklightBrightness(0);
    digitalWrite(BOARD_PWR_EN, LOW);
    pinMode(BOARD_USER_KEY, INPUT_PULLUP);
    rtc_gpio_pullup_en(static_cast<gpio_num_t>(BOARD_USER_KEY));
    rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(BOARD_USER_KEY));
    esp_sleep_enable_ext1_wakeup(1ULL << BOARD_USER_KEY, ESP_EXT1_WAKEUP_ANY_LOW);
    Serial.flush();
    delay(20);
    esp_deep_sleep_start();
}

#define tft gSubPageGfx
#include "page_battery.h"
#include "page_cc1101.h"
#include "page_nfc.h"
#include "page_nrf24.h"
#include "page_sd.h"
#include "page_wifi.h"
#include "page_tft.h"
#include "page_ws2812.h"
#include "page_setting.h"
#undef tft

#include "page_ir.h"
#include "page_mic.h"


void setup()
{
    Serial.begin(115200);
    delay(400);
    Serial.println();
    Serial.println(F("T-Embed CC1101 Factory"));

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);
    board_spi_init_shared_bus();

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(100000U);
    Wire.setTimeOut(20);

    loadFactorySettings();

    g.encBtn.pin = ENCODER_KEY;
    g.usrBtn.pin = BOARD_USER_KEY;
    pinMode(g.encBtn.pin, INPUT_PULLUP);
    pinMode(g.usrBtn.pin, INPUT_PULLUP);
    pinMode(kEncA, INPUT_PULLUP);
    pinMode(kEncB, INPUT_PULLUP);
    g.encBtn.pressed = (digitalRead(g.encBtn.pin) == LOW);
    g.usrBtn.pressed = (digitalRead(g.usrBtn.pin) == LOW);
    g.encBtn.lastChangeMs = millis();
    g.usrBtn.lastChangeMs = millis();
    g.encPrevAB = static_cast<uint8_t>((digitalRead(kEncA) << 1) | digitalRead(kEncB));
    attachInterrupt(digitalPinToInterrupt(kEncA), onEncoderChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(kEncB), onEncoderChange, CHANGE);

    pinMode(DISPLAY_BL, OUTPUT);
    setBacklightBrightness(0);
    board_prepare_display();
    tft.init();
    tft.setRotation(gSettings.rotation);
    tft.fillScreen(TFT_BLACK);
    board_spi_deselect_all();
    setBacklightBrightness(255);

    initMainMenuCanvas();
    g.encLast = g.encRaw;
    g.encActivitySnapshot = g.encRaw;
    noteUserActivity();
    renderMenu();
}

void loop()
{
    pollButton(g.encBtn);
    pollButton(g.usrBtn);
    trackUserActivity();

    if (g.activePage == PageId::MainMenu) {
        const int32_t cur = g.encRaw;
        const int32_t delta = (cur - g.encLast) / 2;
        if (delta != 0) {
            g.encLast += delta * 2;
            int32_t c = static_cast<int32_t>(g.menuCursor) + delta;
            c %= static_cast<int32_t>(kPageCount);
            if (c < 0) {
                c += kPageCount;
            }
            g.menuCursor = static_cast<int8_t>(c);
            g.menuDirty = true;
        }

        if (g.encBtn.event) {
            g.encBtn.event = false;
            enterSubPage(static_cast<PageId>(g.menuCursor + 1));
            return;
        }

        if (g.usrBtn.event) {
            g.usrBtn.event = false;
            g.menuCursor = (g.menuCursor + 1) % kPageCount;
            g.menuDirty = true;
        }

        if (g.menuDirty) {
            renderMenu();
        }
    } else {
        const uint8_t idx = static_cast<uint8_t>(g.activePage) - 1;
        if (idx < kPageCount) {
            kPages[idx].update();
            if (g.subPageExitRequested) {
                exitSubPage();
                return;
            }
            kPages[idx].render();
        }
    }

    handleIdlePowerState();
    if (g.systemSleepRequested) {
        enterSystemSleepNow();
        return;
    }

    delay(g.activePage == PageId::Mic ? 2 : 5);
}
