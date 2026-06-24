#pragma once

namespace page_ir {

namespace {
struct IrPreset {
    const char* name;
    const char* protocolName;
    decode_type_t protocol;
    uint64_t value;
    uint16_t bits;
};

struct RxInfo {
    bool valid = false;
    String protocol = "--";
    String valueHex = "--";
    uint16_t bits = 0;
    uint32_t count = 0;
    uint32_t lastMs = 0;
};

struct TxInfo {
    bool valid = false;
    String name = "--";
    String protocol = "--";
    String valueHex = "--";
    uint16_t bits = 0;
    uint32_t count = 0;
    uint32_t lastMs = 0;
};

struct LedFlash {
    bool active = false;
    uint32_t endMs = 0;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

enum class FooterFocus : uint8_t {
    None = 0,
    Loop,
    Back,
};

constexpr IrPreset kPresets[] = {
    {"NEC  A",  "NEC",     NEC,     0x20DF10EFULL, 32},
    {"NEC  B",  "NEC",     NEC,     0x20DF40BFULL, 32},
    {"Sony12",  "SONY",    SONY,    0x00000A90ULL, 12},
    {"Samsung", "SAMSUNG", SAMSUNG, 0xE0E040BFULL, 32},
    {"RC5",     "RC5",     RC5,     0x00000010ULL, 12},
};
constexpr uint8_t kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

constexpr uint16_t kIrCaptureBufSize = 1024;
constexpr uint8_t kIrTimeoutMs = 50;
constexpr uint32_t kLoopbackIntervalMs = 1500;
constexpr uint32_t kLedFlashMs = 120;
constexpr uint32_t kEchoWindowMs = 250;
constexpr uint32_t kFrameMs = 60;
constexpr uint8_t kLoopFlashLed = 180;

constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kHeader = 0x04FF;
constexpr uint16_t kPanelTx = 0x18E3;
constexpr uint16_t kPanelRx = 0x12CB;
constexpr uint16_t kLabel = TFT_DARKGREY;
constexpr uint16_t kLoopBg = 0x6200;
constexpr uint16_t kLoopAccent = 0xC81F;

constexpr int16_t kHeaderHeight = 22;
constexpr int16_t kBannerY = 24;
constexpr int16_t kBannerH = 24;
constexpr int16_t kBannerW = 220;
constexpr int16_t kPanelTxY = 54;
constexpr int16_t kPanelRxY = 102;
constexpr int16_t kPanelH = 44;
constexpr int16_t kFooterButtonY = 154;
constexpr int16_t kFooterButtonW = 58;
constexpr int16_t kFooterButtonH = 14;
constexpr int16_t kLoopButtonX = 190;
constexpr int16_t kBackButtonX = 256;

IRsend irsend(BOARD_IR_TX);
IRrecv irrecv(BOARD_IR_RX, kIrCaptureBufSize, kIrTimeoutMs, true);
decode_results irRx;
Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSprite canvas(&tft);

uint8_t presetIndex = 0;
bool loopbackMode = false;
uint32_t lastTxMs = 0;
uint32_t lastLoopbackMs = 0;
uint32_t lastAgeTickMs = 0;
uint32_t lastDrawMs = 0;
bool screenDirty = true;
bool canvasReady = false;
int32_t encSnapshot = 0;
FooterFocus footerFocus = FooterFocus::None;

RxInfo rxInfo;
TxInfo txInfo;
LedFlash ledFlash;

String clipText(String text, const uint8_t maxChars)
{
    if (text.length() <= maxChars) {
        return text;
    }
    return text.substring(0, maxChars - 3) + "...";
}

String fmtElapsed(const uint32_t lastMs)
{
    if (lastMs == 0) {
        return "--";
    }

    const uint32_t sec = (millis() - lastMs) / 1000U;
    if (sec < 60U) {
        return String(sec) + "s ago";
    }
    if (sec < 3600U) {
        return String(sec / 60U) + "m ago";
    }
    return String(sec / 3600U) + "h ago";
}

void setStripColor(const uint8_t r, const uint8_t g, const uint8_t b)
{
    for (uint8_t i = 0; i < WS2812_NUM_LEDS; ++i) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void applyIdleLedState()
{
    setStripColor(0, 0, 0);
}

void triggerLedFlash(const uint8_t r, const uint8_t g, const uint8_t b)
{
    ledFlash.active = true;
    ledFlash.endMs = millis() + kLedFlashMs;
    ledFlash.r = r;
    ledFlash.g = g;
    ledFlash.b = b;
    setStripColor(r, g, b);
}

void pollLedFlash()
{
    if (!ledFlash.active || millis() < ledFlash.endMs) {
        return;
    }

    ledFlash.active = false;
    applyIdleLedState();
}

bool sendPreset(const IrPreset& preset)
{
    switch (preset.protocol) {
        case NEC:
            irsend.sendNEC(preset.value, preset.bits);
            return true;
        case SONY:
            irsend.sendSony(preset.value, preset.bits, 2);
            return true;
        case SAMSUNG:
            irsend.sendSAMSUNG(preset.value, preset.bits);
            return true;
        case RC5:
            irsend.sendRC5(preset.value, preset.bits);
            return true;
        default:
            return false;
    }
}

void selectPreset(const uint8_t index)
{
    if (index >= kPresetCount || index == presetIndex) {
        return;
    }

    presetIndex = index;
    screenDirty = true;
    Serial.print(F("[IR] Preset -> "));
    Serial.println(kPresets[presetIndex].name);
}

void doSendCurrentPreset()
{
    const IrPreset& preset = kPresets[presetIndex];

    Serial.print(F("[IR] TX -> "));
    Serial.print(preset.name);
    Serial.print(F("  proto="));
    Serial.print(preset.protocolName);
    Serial.print(F("  value=0x"));
    Serial.print(uint64ToString(preset.value, 16));
    Serial.print(F("  bits="));
    Serial.println(preset.bits);

    if (!sendPreset(preset)) {
        Serial.println(F("[IR] Unsupported preset protocol."));
        return;
    }

    txInfo.valid = true;
    txInfo.name = preset.name;
    txInfo.protocol = preset.protocolName;
    txInfo.valueHex = "0x" + String(uint64ToString(preset.value, 16));
    txInfo.bits = preset.bits;
    txInfo.lastMs = millis();
    lastTxMs = txInfo.lastMs;
    ++txInfo.count;

    screenDirty = true;
    if (loopbackMode) {
        triggerLedFlash(kLoopFlashLed, 0, kLoopFlashLed);
    } else {
        triggerLedFlash(0, 0, 80);
    }
}

void toggleLoopback()
{
    loopbackMode = !loopbackMode;
    lastLoopbackMs = 0;
    screenDirty = true;
    ledFlash.active = false;
    applyIdleLedState();

    Serial.print(F("[IR] Loopback mode: "));
    Serial.println(loopbackMode ? F("ON") : F("OFF"));
}

void pollIrReceive()
{
    if (!irrecv.decode(&irRx)) {
        return;
    }

    const uint32_t now = millis();
    const bool isSelfEcho = (lastTxMs != 0U) && ((now - lastTxMs) < kEchoWindowMs);
    if (isSelfEcho && !loopbackMode) {
        irrecv.resume();
        return;
    }

    rxInfo.valid = (irRx.decode_type != UNKNOWN) && (irRx.bits > 0U);
    rxInfo.protocol = String(typeToString(irRx.decode_type, irRx.repeat));
    rxInfo.valueHex = "0x" + String(uint64ToString(irRx.value, 16));
    rxInfo.bits = irRx.bits;
    rxInfo.lastMs = now;
    ++rxInfo.count;

    Serial.print(F("[IR] RX  proto="));
    Serial.print(rxInfo.protocol);
    Serial.print(F("  value="));
    Serial.print(rxInfo.valueHex);
    Serial.print(F("  bits="));
    Serial.print(rxInfo.bits);
    if (isSelfEcho) {
        Serial.print(F("  (loopback echo)"));
    }
    Serial.println();

    if (loopbackMode || isSelfEcho) {
        triggerLedFlash(kLoopFlashLed, 0, kLoopFlashLed);
    } else {
        triggerLedFlash(0, 80, 0);
    }
    irrecv.resume();
    screenDirty = true;
}

void pollLoopback()
{
    if (!loopbackMode) {
        return;
    }

    const uint32_t now = millis();
    if ((now - lastLoopbackMs) >= kLoopbackIntervalMs) {
        lastLoopbackMs = now;
        doSendCurrentPreset();
    }
}

bool updateFooterFocus()
{
    const int32_t delta = takeEncoderDelta(encSnapshot);
    if (delta == 0) {
        return false;
    }

    int32_t next = static_cast<int32_t>(footerFocus) + delta;
    if (next < static_cast<int32_t>(FooterFocus::None)) {
        next = static_cast<int32_t>(FooterFocus::None);
    }
    if (next > static_cast<int32_t>(FooterFocus::Back)) {
        next = static_cast<int32_t>(FooterFocus::Back);
    }

    const FooterFocus newFocus = static_cast<FooterFocus>(next);
    if (newFocus == footerFocus) {
        return false;
    }
    footerFocus = newFocus;
    return true;
}

template <typename Canvas>
void drawHeader(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderHeight, kHeader);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, kHeader);
    gfx.drawCentreString("IR TX / RX Test", gfx.width() / 2, 4, 2);

    if (loopbackMode) {
        gfx.fillRoundRect(gfx.width() - 70, 3, 66, 16, 4, kLoopBg);
        gfx.setTextColor(TFT_YELLOW, kLoopBg);
        gfx.drawCentreString("LOOP", gfx.width() - 37, 6, 1);
    }
}

template <typename Canvas>
void drawPresetBanner(Canvas& gfx)
{
    const int16_t centerX = gfx.width() / 2;
    const int16_t x = centerX - kBannerW / 2;
    gfx.fillRoundRect(x, kBannerY, kBannerW, kBannerH, 8, TFT_DARKCYAN);
    gfx.drawRoundRect(x, kBannerY, kBannerW, kBannerH, 8, TFT_WHITE);

    const String banner =
        "[" + String(static_cast<unsigned>(presetIndex + 1U)) + "/" +
        String(static_cast<unsigned>(kPresetCount)) + "] " +
        kPresets[presetIndex].name + "  " + kPresets[presetIndex].protocolName;

    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKCYAN);
    gfx.drawString(banner, centerX, kBannerY + (kBannerH / 2) - 1, 2);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawTxPanel(Canvas& gfx)
{
    const int16_t x = 6;
    const int16_t w = gfx.width() - 12;
    const int16_t y = kPanelTxY;
    gfx.fillRoundRect(x, y, w, kPanelH, 6, kPanelTx);
    gfx.drawRoundRect(x, y, w, kPanelH, 6, TFT_DARKGREY);

    gfx.setTextColor(TFT_ORANGE, kPanelTx);
    gfx.drawString("TX", x + 8, y + 3, 2);
    gfx.setTextColor(TFT_CYAN, kPanelTx);
    gfx.drawString(txInfo.valid ? txInfo.protocol : "--", x + 60, y + 6, 1);

    gfx.setTextColor(TFT_WHITE, kPanelTx);
    gfx.drawString(txInfo.valid ? txInfo.name : "--", x + 8, y + 18, 2);
    gfx.drawRightString(clipText(txInfo.valueHex, 18), x + w - 8, y + 18, 1);

    char meta[40];
    snprintf(meta, sizeof(meta), "bits:%u  x%lu",
             static_cast<unsigned>(txInfo.bits),
             static_cast<unsigned long>(txInfo.count));
    gfx.setTextColor(TFT_YELLOW, kPanelTx);
    gfx.drawString(meta, x + 8, y + 33, 1);
    gfx.drawRightString(fmtElapsed(txInfo.lastMs), x + w - 8, y + 33, 1);
}

template <typename Canvas>
void drawRxPanel(Canvas& gfx)
{
    const int16_t x = 6;
    const int16_t w = gfx.width() - 12;
    const int16_t y = kPanelRxY;
    gfx.fillRoundRect(x, y, w, kPanelH, 6, kPanelRx);
    gfx.drawRoundRect(x, y, w, kPanelH, 6, TFT_DARKGREY);

    gfx.setTextColor(TFT_GREENYELLOW, kPanelRx);
    gfx.drawString("RX", x + 8, y + 3, 2);
    gfx.setTextColor(TFT_WHITE, kPanelRx);
    gfx.drawString(rxInfo.valid ? clipText(rxInfo.protocol, 18) : String("waiting..."), x + 60, y + 3, 2);

    if (loopbackMode && rxInfo.valid && txInfo.valid && rxInfo.valueHex == txInfo.valueHex) {
        gfx.fillRoundRect(x + w - 42, y + 3, 34, 14, 3, TFT_DARKGREEN);
        gfx.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        gfx.drawCentreString("OK", x + w - 25, y + 6, 1);
    }

    gfx.setTextColor(TFT_WHITE, kPanelRx);
    gfx.drawString(clipText(rxInfo.valueHex, 22), x + 8, y + 18, 1);

    char meta[40];
    snprintf(meta, sizeof(meta), "bits:%u  x%lu",
             static_cast<unsigned>(rxInfo.bits),
             static_cast<unsigned long>(rxInfo.count));
    gfx.setTextColor(TFT_YELLOW, kPanelRx);
    gfx.drawString(meta, x + 8, y + 33, 1);
    gfx.drawRightString(fmtElapsed(rxInfo.lastMs), x + w - 8, y + 33, 1);
}

template <typename Canvas>
void drawActionButton(Canvas& gfx,
                      const int16_t x,
                      const char* label,
                      const bool selected,
                      const bool active,
                      const uint16_t activeColor)
{
    uint16_t bg = TFT_DARKGREY;
    uint16_t fg = TFT_LIGHTGREY;
    uint16_t border = 0x52AA;

    if (active) {
        bg = activeColor;
        fg = TFT_WHITE;
        border = activeColor;
    }
    if (selected) {
        if (active) {
            border = TFT_YELLOW;
            fg = TFT_BLACK;
        } else {
            bg = TFT_WHITE;
            fg = TFT_BLACK;
            border = TFT_YELLOW;
        }
    }

    gfx.fillRoundRect(x, kFooterButtonY, kFooterButtonW, kFooterButtonH, 5, bg);
    gfx.drawRoundRect(x, kFooterButtonY, kFooterButtonW, kFooterButtonH, 5, border);
    gfx.setTextColor(fg, bg);
    gfx.drawCentreString(label, x + kFooterButtonW / 2, kFooterButtonY + 3, 1);
}

template <typename Canvas>
void drawFooter(Canvas& gfx)
{
    const int16_t y = gfx.height() - kFooterH;
    gfx.fillRect(0, y, gfx.width(), kFooterH, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    if (footerFocus == FooterFocus::Back) {
        gfx.drawString("BOOT back", kMargin, y + 4, 1);
    } else if (footerFocus == FooterFocus::Loop) {
        gfx.drawString(loopbackMode ? "BOOT loop off" : "BOOT loop on", kMargin, y + 4, 1);
    } else if (loopbackMode) {
        gfx.drawString("USER preset  LOOP active", kMargin, y + 4, 1);
    } else {
        gfx.drawString("USER preset  BOOT send", kMargin, y + 4, 1);
    }
    drawActionButton(gfx, kLoopButtonX, "LOOP", footerFocus == FooterFocus::Loop, loopbackMode, kLoopAccent);
    drawActionButton(gfx, kBackButtonX, "BACK", footerFocus == FooterFocus::Back, false, TFT_DARKGREY);
}

template <typename Canvas>
void drawUi(Canvas& gfx)
{
    gfx.fillScreen(kBg);
    drawHeader(gfx);
    drawPresetBanner(gfx);
    drawTxPanel(gfx);
    drawRxPanel(gfx);
    drawFooter(gfx);
}

void redrawAll()
{
    board_prepare_display();
    if (canvasReady) {
        drawUi(canvas);
        canvas.pushSprite(0, 0);
    } else {
        drawUi(tft);
    }
    board_spi_deselect_all();
}

void printHelp()
{
    Serial.println();
    Serial.println(F("IR send/receive test commands:"));
    Serial.println(F("  help         - show this help"));
    Serial.println(F("  status       - show current preset & counters"));
    Serial.println(F("  send         - transmit current preset"));
    Serial.println(F("  next / prev  - cycle preset"));
    Serial.println(F("  preset <n>   - select preset (1..N)"));
    Serial.println(F("  loopback     - toggle self-loopback mode"));
    Serial.println(F("Hardware controls:"));
    Serial.println(F("  USER key     - short press cycles preset"));
    Serial.println(F("  BOOT key     - send / toggle selected action"));
    Serial.println(F("  Encoder      - focus NONE -> LOOP -> BACK"));
    Serial.println();
}

void printStatus()
{
    const IrPreset& preset = kPresets[presetIndex];
    Serial.println();
    Serial.print(F("[IR] TX pin:     GPIO"));
    Serial.println(BOARD_IR_TX);
    Serial.print(F("[IR] RX pin:     GPIO"));
    Serial.println(BOARD_IR_RX);
    Serial.print(F("[IR] Preset:     "));
    Serial.print(presetIndex + 1U);
    Serial.print('/');
    Serial.print(kPresetCount);
    Serial.print(F("  "));
    Serial.println(preset.name);
    Serial.print(F("[IR] Protocol:   "));
    Serial.println(preset.protocolName);
    Serial.print(F("[IR] TX count:   "));
    Serial.println(txInfo.count);
    Serial.print(F("[IR] RX count:   "));
    Serial.println(rxInfo.count);
    Serial.print(F("[IR] Loopback:   "));
    Serial.println(loopbackMode ? F("ON") : F("OFF"));
}

void handleCommand(String line)
{
    line.trim();
    if (line.isEmpty()) {
        return;
    }

    if (line.equalsIgnoreCase("help")) {
        printHelp();
        return;
    }
    if (line.equalsIgnoreCase("status")) {
        printStatus();
        return;
    }
    if (line.equalsIgnoreCase("send")) {
        doSendCurrentPreset();
        return;
    }
    if (line.equalsIgnoreCase("next")) {
        selectPreset(static_cast<uint8_t>((presetIndex + 1U) % kPresetCount));
        return;
    }
    if (line.equalsIgnoreCase("prev")) {
        selectPreset(static_cast<uint8_t>((presetIndex + kPresetCount - 1U) % kPresetCount));
        return;
    }
    if (line.equalsIgnoreCase("loopback")) {
        toggleLoopback();
        return;
    }
    if (line.startsWith("preset ")) {
        const long value = line.substring(7).toInt();
        if (value < 1 || value > kPresetCount) {
            Serial.print(F("[IR] preset must be 1.."));
            Serial.println(kPresetCount);
            return;
        }
        selectPreset(static_cast<uint8_t>(value - 1));
        return;
    }

    Serial.print(F("[IR] Unknown command: "));
    Serial.println(line);
    printHelp();
}

void pollSerialCommands()
{
    if (!Serial.available()) {
        return;
    }
    handleCommand(Serial.readStringUntil('\n'));
}
}  // namespace

void init()
{
    presetIndex = 0;
    loopbackMode = false;
    lastTxMs = 0;
    lastLoopbackMs = 0;
    lastAgeTickMs = millis();
    lastDrawMs = 0;
    screenDirty = true;
    canvasReady = false;
    encSnapshot = g.encRaw;
    footerFocus = FooterFocus::None;
    rxInfo = {};
    txInfo = {};
    ledFlash = {};

    strip.begin();
    strip.setBrightness(60);
    applyIdleLedState();

    irsend.begin();
    irrecv.enableIRIn();
    canvas.setColorDepth(16);
    canvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!canvasReady) {
        Serial.println(F("[IR] Sprite allocation failed, using direct TFT redraw."));
    }

    printStatus();
    printHelp();
}

void update()
{
    if (updateFooterFocus()) {
        screenDirty = true;
    }

    pollSerialCommands();

    if (takeUserButton()) {
        selectPreset(static_cast<uint8_t>((presetIndex + 1U) % kPresetCount));
    }

    if (takeEncoderButton()) {
        if (footerFocus == FooterFocus::Back) {
            requestExitSubPage();
            return;
        }
        if (footerFocus == FooterFocus::Loop) {
            toggleLoopback();
            return;
        }
        if (!loopbackMode) {
            doSendCurrentPreset();
        }
    }

    pollIrReceive();
    pollLoopback();
    pollLedFlash();

    if ((millis() - lastAgeTickMs) >= 1000U) {
        lastAgeTickMs = millis();
        screenDirty = true;
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0U && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    redrawAll();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    irrecv.disableIRIn();
    canvas.deleteSprite();
    canvasReady = false;
    setStripColor(0, 0, 0);
}

}  // namespace page_ir
