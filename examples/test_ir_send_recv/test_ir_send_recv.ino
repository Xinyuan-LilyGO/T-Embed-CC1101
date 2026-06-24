#include <Arduino.h>
#include <TFT_eSPI.h>

#include <Adafruit_NeoPixel.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

#include "utilities.h"

namespace {

// ---- pins ----
constexpr uint8_t kIrTxPin = BOARD_IR_TX;
constexpr uint8_t kIrRxPin = BOARD_IR_RX;
constexpr uint8_t kUsrKey = BOARD_USER_KEY;
constexpr uint8_t kEncA = ENCODER_INA;
constexpr uint8_t kEncB = ENCODER_INB;
constexpr uint8_t kEncKey = ENCODER_KEY;
constexpr uint8_t kLedPin = WS2812_DATA_PIN;
constexpr uint8_t kLedCount = WS2812_NUM_LEDS;

// ---- display ----
constexpr uint8_t kRotation = 3;
constexpr uint32_t kBusSettleMs = 20;

// ---- IR ----
constexpr uint16_t kIrCaptureBufSize = 1024;
constexpr uint8_t kIrTimeoutMs = 50;
constexpr uint32_t kDebounceMs = 20;

// ---- loopback ----
constexpr uint32_t kLoopbackIntervalMs = 1500;

// ---- LED flash ----
constexpr uint32_t kLedFlashMs = 120;

// ---- echo suppression ----
constexpr uint32_t kEchoWindowMs = 250;

struct IrPreset {
    const char *name;
    const char *protocolName;
    decode_type_t protocol;
    uint64_t value;
    uint16_t bits;
};

const IrPreset kPresets[] = {
    {"NEC  A", "NEC", NEC, 0x20DF10EFULL, 32},
    {"NEC  B", "NEC", NEC, 0x20DF40BFULL, 32},
    {"Sony12", "SONY", SONY, 0x00000A90ULL, 12},
    {"Samsung", "SAMSUNG", SAMSUNG, 0xE0E040BFULL, 32},
    {"RC5", "RC5", RC5, 0x00000010ULL, 12},
};
constexpr uint8_t kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

TFT_eSPI tft;
IRsend irsend(kIrTxPin);
IRrecv irrecv(kIrRxPin, kIrCaptureBufSize, kIrTimeoutMs, true);
decode_results irRx;
Adafruit_NeoPixel leds(kLedCount, kLedPin, NEO_GRB + NEO_KHZ800);

uint8_t presetIndex = 0;
bool loopbackMode = false;
uint32_t lastTxMs = 0;

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

RxInfo rxInfo;
TxInfo txInfo;

struct LedFlash {
    bool active = false;
    uint32_t endMs = 0;
};
LedFlash ledFlash;

struct ButtonState {
    uint8_t pin;
    bool pressed;
    bool pressedEvent;
    uint32_t lastChangeMs;
};

ButtonState usrBtn = {kUsrKey, false, false, 0};
ButtonState encBtn = {kEncKey, false, false, 0};

struct Dirty {
    bool chrome = true;
    bool header = true;
    bool preset = true;
    bool tx = true;
    bool rx = true;
    bool txTime = true;
    bool rxTime = true;
};
Dirty dirty;

volatile int32_t encoderCount = 0;
volatile uint8_t prevAB = 0;
static const int8_t kEncTable[4][4] = {
    {0, -1, 1, 0},
    {1, 0, 0, -1},
    {-1, 0, 0, 1},
    {0, 1, -1, 0},
};

constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kHeader = 0x04FF;
constexpr uint16_t kPanelTx = 0x18E3;
constexpr uint16_t kPanelRx = 0x12CB;
constexpr uint16_t kLabel = TFT_DARKGREY;
constexpr uint16_t kLoopBg = 0x6200;

constexpr int16_t kHeaderH = 22;
constexpr int16_t kBannerY = 24;
constexpr int16_t kBannerH = 26;
constexpr int16_t kBannerW = 220;
constexpr int16_t kPanelTxY = 53;
constexpr int16_t kPanelRxY = 112;
constexpr int16_t kPanelH = 56;

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

bool initDisplayPower()
{
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);

    deselectSharedSpiDevices();
    delay(kBusSettleMs);

    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    tft.begin();
    tft.setRotation(kRotation);
    tft.fillScreen(TFT_BLACK);

    digitalWrite(DISPLAY_BL, HIGH);
    return true;
}

String fmtElapsed(uint32_t lastMs)
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

void startLedFlash(uint8_t r, uint8_t g, uint8_t b)
{
    ledFlash.active = true;
    ledFlash.endMs = millis() + kLedFlashMs;

    for (uint8_t index = 0; index < kLedCount; ++index) {
        leds.setPixelColor(index, leds.Color(r, g, b));
    }
    leds.show();
}

void pollLedFlash()
{
    if (!ledFlash.active || millis() < ledFlash.endMs) {
        return;
    }

    ledFlash.active = false;
    leds.clear();
    leds.show();
}

void IRAM_ATTR onEncoderChange()
{
    const uint8_t a = digitalRead(kEncA);
    const uint8_t b = digitalRead(kEncB);
    const uint8_t cur = static_cast<uint8_t>((a << 1) | b);
    encoderCount += kEncTable[prevAB][cur];
    prevAB = cur;
}

void pollButton(ButtonState &btn)
{
    const bool raw = (digitalRead(btn.pin) == LOW);
    const uint32_t now = millis();

    if (raw != btn.pressed && (now - btn.lastChangeMs) >= kDebounceMs) {
        btn.lastChangeMs = now;
        btn.pressed = raw;
        if (raw) {
            btn.pressedEvent = true;
        }
    }
}

bool sendPreset(const IrPreset &preset)
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

void doSendCurrentPreset()
{
    const IrPreset &preset = kPresets[presetIndex];

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
    lastTxMs = millis();
    ++txInfo.count;

    dirty.tx = true;
    dirty.txTime = true;
    startLedFlash(0, 0, 80);
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

    dirty.rx = true;
    dirty.rxTime = true;

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

    if (isSelfEcho) {
        startLedFlash(80, 0, 80);
    } else {
        startLedFlash(0, 80, 0);
    }

    irrecv.resume();
}

void pollLoopback()
{
    if (!loopbackMode) {
        return;
    }

    static uint32_t lastLoopMs = 0;
    const uint32_t now = millis();
    if (now - lastLoopMs >= kLoopbackIntervalMs) {
        lastLoopMs = now;
        doSendCurrentPreset();
    }
}

void toggleLoopback()
{
    loopbackMode = !loopbackMode;
    dirty.header = true;
    dirty.rx = true;

    Serial.print(F("[IR] Loopback mode: "));
    Serial.println(loopbackMode ? F("ON") : F("OFF"));
}

void drawChrome()
{
    const int16_t width = tft.width();
    tft.fillScreen(kBg);

    tft.fillRoundRect(6, kPanelTxY, width - 12, kPanelH, 6, kPanelTx);
    tft.drawRoundRect(6, kPanelTxY, width - 12, kPanelH, 6, TFT_DARKGREY);
    tft.fillRoundRect(6, kPanelRxY, width - 12, kPanelH, 6, kPanelRx);
    tft.drawRoundRect(6, kPanelRxY, width - 12, kPanelH, 6, TFT_DARKGREY);

    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(0);

    tft.setTextColor(TFT_ORANGE, kPanelTx);
    tft.drawString("TX", 12, kPanelTxY + 4, 2);
    tft.setTextColor(kLabel, kPanelTx);
    tft.drawString("val", 12, kPanelTxY + 24, 1);

    tft.setTextColor(TFT_GREENYELLOW, kPanelRx);
    tft.drawString("RX", 12, kPanelRxY + 4, 2);
    tft.setTextColor(kLabel, kPanelRx);
    tft.drawString("val", 12, kPanelRxY + 24, 1);
}

void drawHeader()
{
    const int16_t width = tft.width();
    tft.fillRect(0, 0, width, kHeaderH, kHeader);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, kHeader);
    tft.setTextPadding(0);
    tft.drawCentreString("IR TX / RX Test", width / 2, 4, 2);

    if (loopbackMode) {
        tft.fillRoundRect(width - 70, 3, 66, 16, 4, kLoopBg);
        tft.setTextColor(TFT_YELLOW, kLoopBg);
        tft.drawCentreString("LOOP", width - 37, 6, 1);
    }
}

void drawPresetBanner()
{
    const int16_t width = tft.width();
    const int16_t centerX = width / 2;
    const int16_t x = centerX - kBannerW / 2;

    tft.fillRoundRect(x, kBannerY, kBannerW, kBannerH, 8, TFT_DARKCYAN);
    tft.drawRoundRect(x, kBannerY, kBannerW, kBannerH, 8, TFT_WHITE);

    const String banner =
        "[" + String(static_cast<unsigned>(presetIndex + 1U)) + "/" +
        String(static_cast<unsigned>(kPresetCount)) + "] " +
        kPresets[presetIndex].name + "  " + kPresets[presetIndex].protocolName;

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
    tft.setTextPadding(0);
    tft.drawCentreString(banner.c_str(), centerX, kBannerY + 5, 2);
}

void drawTxValues()
{
    const int16_t width = tft.width();
    const int16_t panelX = 6;
    const int16_t panelW = width - 12;
    const int16_t panelY = kPanelTxY;

    tft.setTextDatum(TL_DATUM);

    tft.setTextColor(TFT_WHITE, kPanelTx);
    tft.setTextPadding(70);
    tft.drawString(txInfo.valid ? txInfo.name.c_str() : "--", panelX + 38, panelY + 4, 2);

    tft.setTextColor(TFT_CYAN, kPanelTx);
    tft.setTextPadding(120);
    tft.drawString(txInfo.valid ? txInfo.protocol.c_str() : "", panelX + 110, panelY + 4, 2);

    tft.setTextColor(TFT_WHITE, kPanelTx);
    tft.setTextPadding(panelW - 40);
    tft.drawString(txInfo.valueHex.c_str(), panelX + 28, panelY + 22, 2);

    char buf[24];
    snprintf(buf, sizeof(buf), "bits:%u", static_cast<unsigned>(txInfo.bits));
    tft.setTextColor(kLabel, kPanelTx);
    tft.setTextPadding(60);
    tft.drawString(buf, panelX + 6, panelY + 42, 1);

    snprintf(buf, sizeof(buf), "x%lu", static_cast<unsigned long>(txInfo.count));
    tft.setTextColor(TFT_YELLOW, kPanelTx);
    tft.setTextPadding(60);
    tft.drawString(buf, panelX + 68, panelY + 42, 1);

    tft.setTextPadding(0);
}

void drawTxTime()
{
    const int16_t width = tft.width();
    const int16_t panelX = 6;
    const int16_t panelW = width - 12;
    const int16_t panelY = kPanelTxY;

    const String elapsed = fmtElapsed(txInfo.lastMs);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(kLabel, kPanelTx);
    tft.setTextPadding(70);
    tft.drawString(elapsed.c_str(), panelX + panelW - 4, panelY + 42, 1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(0);
}

void drawRxValues()
{
    const int16_t width = tft.width();
    const int16_t panelX = 6;
    const int16_t panelW = width - 12;
    const int16_t panelY = kPanelRxY;

    tft.setTextDatum(TL_DATUM);

    tft.setTextColor(TFT_WHITE, kPanelRx);
    tft.setTextPadding(160);
    tft.drawString(rxInfo.valid ? rxInfo.protocol.c_str() : "waiting...",
                   panelX + 38, panelY + 4, 2);

    const int16_t badgeX = panelX + panelW - 38;
    const int16_t badgeY = panelY + 2;
    if (loopbackMode && rxInfo.valid && txInfo.valid && rxInfo.valueHex == txInfo.valueHex) {
        tft.fillRoundRect(badgeX, badgeY, 34, 14, 3, TFT_DARKGREEN);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
        tft.setTextPadding(0);
        tft.drawCentreString("OK", badgeX + 17, badgeY + 3, 1);
    } else {
        tft.fillRect(badgeX, badgeY, 34, 14, kPanelRx);
    }

    tft.setTextColor(TFT_WHITE, kPanelRx);
    tft.setTextPadding(panelW - 40);
    tft.drawString(rxInfo.valid ? rxInfo.valueHex.c_str() : "--",
                   panelX + 28, panelY + 22, 2);

    char buf[24];
    snprintf(buf, sizeof(buf), "bits:%u", static_cast<unsigned>(rxInfo.bits));
    tft.setTextColor(kLabel, kPanelRx);
    tft.setTextPadding(60);
    tft.drawString(buf, panelX + 6, panelY + 42, 1);

    snprintf(buf, sizeof(buf), "x%lu", static_cast<unsigned long>(rxInfo.count));
    tft.setTextColor(TFT_YELLOW, kPanelRx);
    tft.setTextPadding(60);
    tft.drawString(buf, panelX + 68, panelY + 42, 1);

    tft.setTextPadding(0);
}

void drawRxTime()
{
    const int16_t width = tft.width();
    const int16_t panelX = 6;
    const int16_t panelW = width - 12;
    const int16_t panelY = kPanelRxY;

    const String elapsed = fmtElapsed(rxInfo.lastMs);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(kLabel, kPanelRx);
    tft.setTextPadding(70);
    tft.drawString(elapsed.c_str(), panelX + panelW - 4, panelY + 42, 1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(0);
}

void render()
{
    if (dirty.chrome) {
        drawChrome();
        dirty.chrome = false;
    }
    if (dirty.header) {
        drawHeader();
        dirty.header = false;
    }
    if (dirty.preset) {
        drawPresetBanner();
        dirty.preset = false;
    }
    if (dirty.tx) {
        drawTxValues();
        dirty.tx = false;
    }
    if (dirty.rx) {
        drawRxValues();
        dirty.rx = false;
    }
    if (dirty.txTime) {
        drawTxTime();
        dirty.txTime = false;
    }
    if (dirty.rxTime) {
        drawRxTime();
        dirty.rxTime = false;
    }
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
    Serial.println();
    Serial.println(F("Buttons:"));
    Serial.println(F("  Encoder rotate -> change preset"));
    Serial.println(F("  Encoder press  -> single send"));
    Serial.println(F("  USER key       -> toggle self-loopback mode"));
}

void printStatus()
{
    const IrPreset &preset = kPresets[presetIndex];

    Serial.println();
    Serial.print(F("[IR] TX pin:     GPIO"));
    Serial.println(kIrTxPin);
    Serial.print(F("[IR] RX pin:     GPIO"));
    Serial.println(kIrRxPin);
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

void selectPreset(uint8_t index)
{
    if (index >= kPresetCount || index == presetIndex) {
        return;
    }

    presetIndex = index;
    dirty.preset = true;

    Serial.print(F("[IR] Preset -> "));
    Serial.println(kPresets[presetIndex].name);
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
        selectPreset((presetIndex + 1U) % kPresetCount);
        return;
    }
    if (line.equalsIgnoreCase("prev")) {
        selectPreset((presetIndex + kPresetCount - 1U) % kPresetCount);
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

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("T-Embed IR send/receive test"));

    pinMode(kUsrKey, INPUT_PULLUP);
    pinMode(kEncKey, INPUT_PULLUP);
    pinMode(kEncA, INPUT_PULLUP);
    pinMode(kEncB, INPUT_PULLUP);

    prevAB = static_cast<uint8_t>((digitalRead(kEncA) << 1) | digitalRead(kEncB));
    attachInterrupt(digitalPinToInterrupt(kEncA), onEncoderChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(kEncB), onEncoderChange, CHANGE);

    leds.begin();
    leds.setBrightness(60);
    leds.clear();
    leds.show();

    if (!initDisplayPower()) {
        Serial.println(F("[IR] Display power init failed. Halting."));
        while (true) {
            delay(1000);
        }
    }

    irsend.begin();
    irrecv.enableIRIn();

    render();
    printStatus();
    printHelp();
}

void loop()
{
    pollSerialCommands();
    pollIrReceive();
    pollLoopback();
    pollLedFlash();
    pollButton(usrBtn);
    pollButton(encBtn);

    if (usrBtn.pressedEvent) {
        usrBtn.pressedEvent = false;
        toggleLoopback();
    }

    if (encBtn.pressedEvent) {
        encBtn.pressedEvent = false;
        if (!loopbackMode) {
            doSendCurrentPreset();
        }
    }

    static int32_t lastEnc = 0;
    const int32_t cur = encoderCount;
    const int32_t delta = (cur - lastEnc) / 2;
    if (delta != 0) {
        lastEnc += delta * 2;
        int32_t index = static_cast<int32_t>(presetIndex) + delta;
        index %= kPresetCount;
        if (index < 0) {
            index += kPresetCount;
        }
        selectPreset(static_cast<uint8_t>(index));
    }

    static uint32_t lastTickMs = 0;
    if (millis() - lastTickMs >= 1000) {
        lastTickMs = millis();
        dirty.txTime = true;
        dirty.rxTime = true;
    }

    render();
    delay(2);
}
