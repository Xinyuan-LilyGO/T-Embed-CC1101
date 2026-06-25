#pragma once

extern SPIClass spi;

namespace page_nrf24 {

namespace {
struct ChannelChoice {
    uint8_t channel;
    const char* label;
};

struct DataRateChoice {
    uint16_t kbps;
    const char* label;
};

struct PowerChoice {
    int8_t dbm;
    const char* label;
};

enum class RadioMode : uint8_t {
    Receive = 0,
    BurstTransmit,
};

enum class LedEffect : uint8_t {
    None = 0,
    RxFlash,
    TxFlash,
};

constexpr ChannelChoice kChannelChoices[] = {
    {0, "CH 000"},
    {40, "CH 040"},
    {76, "CH 076"},
    {125, "CH 125"},
};
constexpr uint8_t kChannelChoiceCount = sizeof(kChannelChoices) / sizeof(kChannelChoices[0]);

constexpr DataRateChoice kDataRateChoices[] = {
    {250, "250K"},
    {1000, "1M"},
    {2000, "2M"},
};

constexpr PowerChoice kPowerChoices[] = {
    {-18, "-18"},
    {-12, "-12"},
    {-6, "-6"},
    {0, "0"},
};

constexpr uint8_t kAddressWidth = 5;
constexpr uint8_t kPipeAddress[kAddressWidth] = {0x01, 0x23, 0x45, 0x67, 0x89};
constexpr uint32_t kBurstIntervalMs = 1000;
constexpr uint32_t kTxTimeoutMs = 100;
constexpr char kDefaultTxPrefix[] = "T-Embed nRF24";

constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;
constexpr int16_t kMarginLeft = 8;
constexpr int16_t kRowParam = 30;
constexpr int16_t kRowMode = 64;
constexpr int16_t kRowDivider = 86;
constexpr int16_t kRowInfo1 = 92;
constexpr int16_t kRowInfo2 = 108;
constexpr int16_t kRowInfo3 = 124;
constexpr int16_t kRowInfo4 = 140;

constexpr uint8_t kLedBrightness = 10;
constexpr uint32_t kLedFlashMs = 200;
constexpr uint32_t kFrameMs = 60;

// The panel setup routes TFT_eSPI through HSPI on ESP32-S3.
// nRF24 must share that same SPI controller instance, otherwise RadioLib
// talks to the wrong bus and radio.begin() fails with SPI write errors.
SPIClass& radioSpi = spi;
nRF24 radio = new Module(BOARD_NRF24_CS, BOARD_NRF24_IRQ, BOARD_NRF24_CE, RADIOLIB_NC, radioSpi);
Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

RadioMode currentMode = RadioMode::Receive;
uint8_t currentChannelIndex = 2;
uint8_t currentChannelValue = 76;
uint8_t currentDataRateIndex = 1;
uint8_t currentPowerIndex = 3;
bool autoAckEnabled = true;

unsigned long lastBurstAtMs = 0;
uint32_t burstCounter = 0;
uint32_t rxCounter = 0;
String burstPrefix = kDefaultTxPrefix;

String lastRxPayload = "";
size_t lastRxLength = 0;
bool hasLastRx = false;
bool radioReady = false;
bool screenDirty = true;
bool backFocused = false;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;

LedEffect ledEffect = LedEffect::None;
uint32_t ledEffectUntilMs = 0;
bool ledDirty = true;

uint8_t currentChannel()
{
    return currentChannelValue;
}

const char* currentChannelLabel()
{
    static char label[12];
    snprintf(label, sizeof(label), "CH %03u", currentChannelValue);
    return label;
}

uint16_t currentDataRateKbps()
{
    return kDataRateChoices[currentDataRateIndex].kbps;
}

const char* currentDataRateLabel()
{
    return kDataRateChoices[currentDataRateIndex].label;
}

int8_t currentOutputPowerDbm()
{
    return kPowerChoices[currentPowerIndex].dbm;
}

const char* currentOutputPowerLabel()
{
    return kPowerChoices[currentPowerIndex].label;
}

float currentFrequencyMHz()
{
    return 2400.0f + static_cast<float>(currentChannel());
}

const char* modeLabel(const RadioMode mode)
{
    switch (mode) {
        case RadioMode::Receive:       return "RX";
        case RadioMode::BurstTransmit: return "TX-BURST";
        default:                       return "?";
    }
}

void triggerLedEffect(const LedEffect effect)
{
    ledEffect = effect;
    ledEffectUntilMs = millis() + kLedFlashMs;
    ledDirty = true;
}

void setStripColor(const uint8_t r, const uint8_t g, const uint8_t b)
{
    for (uint8_t i = 0; i < WS2812_NUM_LEDS; ++i) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void updateLeds()
{
    if (!ledDirty) {
        return;
    }

    ledDirty = false;
    if (ledEffect == LedEffect::None || millis() > ledEffectUntilMs) {
        ledEffect = LedEffect::None;
        setStripColor(0, 0, 0);
        return;
    }

    switch (ledEffect) {
        case LedEffect::RxFlash:
            setStripColor(0, kLedBrightness, 0);
            break;
        case LedEffect::TxFlash:
            setStripColor(0, 0, kLedBrightness);
            break;
        case LedEffect::None:
        default:
            setStripColor(0, 0, 0);
            break;
    }
}

void checkLedTimeout()
{
    if (ledEffect != LedEffect::None && millis() > ledEffectUntilMs) {
        ledDirty = true;
    }
}

void drawHeader()
{
    tft.fillRect(0, 0, tft.width(), kHeaderHeight, TFT_NAVY);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("nRF24 Send / Recv", kMargin, 6, 2);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.drawString("Factory", tft.width() - kMargin, 7, 1);
    tft.setTextDatum(TL_DATUM);
}

void drawFooter()
{
    const int16_t y = tft.height() - kFooterHeight;
    tft.fillRect(0, y, tft.width(), kFooterHeight, TFT_DARKGREY);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    if (!radioReady) {
        tft.drawString("Radio init failed", kMarginLeft, y + 3, 1);
    } else if (backFocused) {
        tft.drawString("BOOT back  USER chan", kMarginLeft, y + 3, 1);
    } else {
        tft.drawString("USER chan  BOOT mode", kMarginLeft, y + 3, 1);
    }
    drawBackButton(backFocused);
}

void drawChannelRow()
{
    tft.fillRect(0, kRowParam, tft.width(), 28, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("CHAN:", kMarginLeft, kRowParam + 8, 1);

    int8_t presetIndex = -1;
    for (uint8_t i = 0; i < kChannelChoiceCount; ++i) {
        if (kChannelChoices[i].channel == currentChannelValue) {
            presetIndex = static_cast<int8_t>(i);
            break;
        }
    }

    uint16_t color = TFT_WHITE;
    if (presetIndex == 0) {
        color = TFT_GREEN;
    } else if (presetIndex == 1) {
        color = TFT_YELLOW;
    } else if (presetIndex == 2) {
        color = TFT_CYAN;
    } else if (presetIndex == 3) {
        color = TFT_ORANGE;
    }

    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(currentChannelLabel(), kMarginLeft + 50, kRowParam, 4);
}

void drawModeRow()
{
    tft.fillRect(0, kRowMode, tft.width(), 18, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("MODE:", kMarginLeft, kRowMode + 2, 1);

    const uint16_t color = currentMode == RadioMode::Receive ? TFT_GREEN : TFT_ORANGE;
    tft.setTextColor(color, TFT_BLACK);
    tft.drawString(currentMode == RadioMode::Receive ? "RX" : "TX BURST", kMarginLeft + 50, kRowMode, 2);
}

void drawDivider()
{
    tft.drawFastHLine(0, kRowDivider, tft.width(), TFT_DARKGREY);
}

void drawConfigSummaryLine(const int16_t y)
{
    char line[64];
    snprintf(line, sizeof(line), "CH%03u %.0fMHz %s %sdBm %s",
             currentChannel(),
             currentFrequencyMHz(),
             currentDataRateLabel(),
             currentOutputPowerLabel(),
             autoAckEnabled ? "ACK" : "NOACK");
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(line, kMarginLeft, y, 1);
}

void drawReceiveRows()
{
    tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Last RX:", kMarginLeft, kRowInfo1, 1);

    if (!hasLastRx) {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("(none)", kMarginLeft + 56, kRowInfo1, 1);
        tft.drawString("Waiting for packets...", kMarginLeft, kRowInfo2, 1);

        char idleMeta[32];
        snprintf(idleMeta, sizeof(idleMeta), "RX count: %lu", static_cast<unsigned long>(rxCounter));
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(idleMeta, kMarginLeft, kRowInfo3, 1);
        drawConfigSummaryLine(kRowInfo4);
        return;
    }

    String payload = lastRxPayload;
    if (payload.length() > 38) {
        payload = payload.substring(0, 35) + "...";
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(payload, kMarginLeft, kRowInfo2, 1);

    char meta[40];
    snprintf(meta, sizeof(meta), "LEN:%u  RX#%lu",
             static_cast<unsigned>(lastRxLength),
             static_cast<unsigned long>(rxCounter));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(meta, kMarginLeft, kRowInfo3, 1);
    drawConfigSummaryLine(kRowInfo4);
}

void drawTransmitRows()
{
    tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);

    char line[48];
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    snprintf(line, sizeof(line), "Burst TX every %lu ms", static_cast<unsigned long>(kBurstIntervalMs));
    tft.drawString(line, kMarginLeft, kRowInfo1, 1);

    snprintf(line, sizeof(line), "TX count: %lu", static_cast<unsigned long>(burstCounter));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(line, kMarginLeft, kRowInfo2, 1);

    String prefix = burstPrefix;
    if (prefix.length() > 28) {
        prefix = prefix.substring(0, 25) + "...";
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Prefix: " + prefix, kMarginLeft, kRowInfo3, 1);
    drawConfigSummaryLine(kRowInfo4);
}

void redrawAll()
{
    board_prepare_display();
    tft.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);
    drawHeader();
    drawChannelRow();
    drawModeRow();
    drawDivider();
    if (currentMode == RadioMode::Receive) {
        drawReceiveRows();
    } else {
        drawTransmitRows();
    }
    drawFooter();
    board_spi_deselect_all();
}

void printHelp()
{
    Serial.println();
    Serial.println(F("nRF24 send/receive test commands:"));
    Serial.println(F("  help               - show this help"));
    Serial.println(F("  status             - show current settings"));
    Serial.println(F("  rx                 - enter receive mode"));
    Serial.println(F("  tx                 - send a test packet every second"));
    Serial.println(F("  send <text>        - send one packet immediately"));
    Serial.println(F("  ch <0-125>         - switch RF channel"));
    Serial.println(F("  rate 250|1000|2000 - switch data rate (kbps)"));
    Serial.println(F("  power -18|-12|-6|0 - switch output power"));
    Serial.println(F("  ack on|off         - set auto ACK"));
    Serial.println(F("  prefix <text>      - change periodic TX message prefix"));
    Serial.println(F("Hardware controls:"));
    Serial.println(F("  USER key           - cycle channel preset"));
    Serial.println(F("  BOOT key           - cycle RX <-> TX"));
    Serial.println(F("  Encoder            - focus BACK"));
    Serial.println();
}

void printStatus()
{
    Serial.println();
    Serial.print(F("[nRF24] Mode:         "));
    Serial.println(modeLabel(currentMode));
    Serial.print(F("[nRF24] Channel:      "));
    Serial.println(currentChannel());
    Serial.print(F("[nRF24] Frequency:    "));
    Serial.print(currentFrequencyMHz(), 0);
    Serial.println(F(" MHz"));
    Serial.print(F("[nRF24] Data rate:    "));
    Serial.print(currentDataRateKbps());
    Serial.println(F(" kbps"));
    Serial.print(F("[nRF24] Output power: "));
    Serial.print(currentOutputPowerDbm());
    Serial.println(F(" dBm"));
    Serial.print(F("[nRF24] Auto ACK:     "));
    Serial.println(autoAckEnabled ? F("ON") : F("OFF"));
    Serial.print(F("[nRF24] TX prefix:    "));
    Serial.println(burstPrefix);
}

void configureNrf24SpiProtocol()
{
    radio.getMod()->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
    radio.getMod()->spiConfig.statusPos = 0;
}

bool applyRadioSettings()
{
    board_spi_deselect_all();
    configureNrf24SpiProtocol();

    int state = radio.begin(static_cast<int16_t>(currentFrequencyMHz()),
                            static_cast<int16_t>(currentDataRateKbps()),
                            currentOutputPowerDbm(),
                            kAddressWidth);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] radio.begin failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setAddressWidth(kAddressWidth);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setAddressWidth failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setFrequency(currentFrequencyMHz());
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setFrequency failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setBitRate(currentDataRateKbps());
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setBitRate failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setOutputPower(currentOutputPowerDbm());
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setOutputPower failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setAutoAck(autoAckEnabled);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setAutoAck failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setReceivePipe(0, kPipeAddress);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setReceivePipe failed, code "));
        Serial.println(state);
        return false;
    }

    state = radio.setTransmitPipe(kPipeAddress);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] setTransmitPipe failed, code "));
        Serial.println(state);
        return false;
    }

    return true;
}

bool initRadio()
{
    board_spi_deselect_all();
    delay(20);

    Serial.print(F("[nRF24] Initializing at CH "));
    Serial.print(currentChannel());
    Serial.print(F(" / "));
    Serial.print(currentFrequencyMHz(), 0);
    Serial.println(F(" MHz ..."));

    return applyRadioSettings();
}

bool enterReceiveMode()
{
    currentMode = RadioMode::Receive;
    screenDirty = true;

    board_spi_deselect_all();
    (void)radio.finishTransmit();
    (void)radio.finishReceive();
    (void)radio.standby();
    (void)radio.setReceivePipe(0, kPipeAddress);

    const int state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("[nRF24] startReceive failed, code "));
        Serial.println(state);
        return false;
    }

    Serial.println(F("[nRF24] Mode switched to RX."));
    return true;
}

void enterBurstTransmitMode()
{
    currentMode = RadioMode::BurstTransmit;
    lastBurstAtMs = 0;
    screenDirty = true;

    board_spi_deselect_all();
    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.standby();
    (void)radio.setTransmitPipe(kPipeAddress);

    Serial.println(F("[nRF24] Mode switched to TX burst."));
}

bool transmitBlocking(const String& payload)
{
    board_spi_deselect_all();
    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.standby();
    (void)radio.setTransmitPipe(kPipeAddress);

    Serial.print(F("[nRF24] TX -> "));
    Serial.println(payload);

    int state = radio.startTransmit(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), 0);
    if (state != RADIOLIB_ERR_NONE) {
        if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
            Serial.println(F("[nRF24] TX failed: packet too long."));
        } else {
            Serial.print(F("[nRF24] startTransmit failed, code "));
            Serial.println(state);
        }
        (void)radio.finishTransmit();
        return false;
    }

    const uint32_t startedAtMs = millis();
    while ((millis() - startedAtMs) < kTxTimeoutMs) {
        const int txDone = radio.getStatus(RADIOLIB_NRF24_TX_DS);
        const int txFailed = radio.getStatus(RADIOLIB_NRF24_MAX_RT);
        if (txFailed > 0) {
            Serial.println(F("[nRF24] TX failed: ACK not received."));
            (void)radio.finishTransmit();
            return false;
        }
        if (txDone > 0) {
            Serial.println(F("[nRF24] TX success."));
            (void)radio.finishTransmit();
            triggerLedEffect(LedEffect::TxFlash);
            return true;
        }
        delay(1);
    }

    Serial.println(F("[nRF24] TX failed: timeout."));
    (void)radio.finishTransmit();
    return false;
}

bool sendOnePacket(const String& payload, const bool resumeRx)
{
    const bool ok = transmitBlocking(payload);
    if (resumeRx) {
        return enterReceiveMode() && ok;
    }
    return ok;
}

bool reinitializeRadioForCurrentMode()
{
    board_spi_deselect_all();
    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.sleep();
    delay(10);

    if (!initRadio()) {
        radioReady = false;
        screenDirty = true;
        return false;
    }

    radioReady = true;
    if (currentMode == RadioMode::Receive) {
        return enterReceiveMode();
    }
    enterBurstTransmitMode();
    return true;
}

void handleReceivedPacket()
{
    if (currentMode != RadioMode::Receive) {
        return;
    }

    board_spi_deselect_all();
    if (radio.getStatus(RADIOLIB_NRF24_RX_DR) <= 0) {
        return;
    }

    lastRxLength = radio.getPacketLength();
    if (lastRxLength > RADIOLIB_NRF24_MAX_PACKET_LENGTH) {
        Serial.print(F("[nRF24] Invalid RX payload length: "));
        Serial.println(static_cast<unsigned>(lastRxLength));
        (void)radio.finishReceive();
        (void)radio.startReceive();
        return;
    }

    uint8_t buffer[RADIOLIB_NRF24_MAX_PACKET_LENGTH + 1] = {0};
    const int state = radio.readData(buffer, 0);
    if (state == RADIOLIB_ERR_NONE) {
        String payload;
        if (lastRxLength == 0U) {
            payload = "(empty)";
        } else {
            payload.reserve(lastRxLength);
            for (size_t i = 0; i < lastRxLength; ++i) {
                const char c = static_cast<char>(buffer[i]);
                payload += (c == '\r' || c == '\n' || c == '\t') ? ' ' : c;
            }
        }

        rxCounter++;
        lastRxPayload = payload;
        hasLastRx = true;
        screenDirty = true;
        triggerLedEffect(LedEffect::RxFlash);

        Serial.println(F("[nRF24] RX packet received."));
        Serial.print(F("[nRF24] Data: "));
        Serial.println(payload);
        Serial.print(F("[nRF24] Length: "));
        Serial.println(static_cast<unsigned>(lastRxLength));
    } else {
        Serial.print(F("[nRF24] RX readData failed, code "));
        Serial.println(state);
    }

    board_spi_deselect_all();
    (void)radio.startReceive();
}

bool parseChannelValue(const String& input, uint8_t& outChannel)
{
    String trimmed = input;
    trimmed.trim();
    if (trimmed.isEmpty()) {
        return false;
    }
    const int value = trimmed.toInt();
    if (value < 0 || value > 125) {
        return false;
    }
    outChannel = static_cast<uint8_t>(value);
    return true;
}

bool parseDataRateToIndex(const String& input, uint8_t& outIndex)
{
    const int value = input.toInt();
    for (uint8_t i = 0; i < sizeof(kDataRateChoices) / sizeof(kDataRateChoices[0]); ++i) {
        if (kDataRateChoices[i].kbps == value) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

bool parsePowerToIndex(const String& input, uint8_t& outIndex)
{
    const int value = input.toInt();
    for (uint8_t i = 0; i < sizeof(kPowerChoices) / sizeof(kPowerChoices[0]); ++i) {
        if (kPowerChoices[i].dbm == value) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

bool parseAckSetting(const String& input, bool& outAckEnabled)
{
    String trimmed = input;
    trimmed.trim();
    trimmed.toLowerCase();
    if (trimmed == "on" || trimmed == "1" || trimmed == "true") {
        outAckEnabled = true;
        return true;
    }
    if (trimmed == "off" || trimmed == "0" || trimmed == "false") {
        outAckEnabled = false;
        return true;
    }
    return false;
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
    if (line.equalsIgnoreCase("rx")) {
        (void)enterReceiveMode();
        return;
    }
    if (line.equalsIgnoreCase("tx")) {
        enterBurstTransmitMode();
        return;
    }
    if (line.startsWith("send ")) {
        String payload = line.substring(5);
        payload.trim();
        if (payload.isEmpty()) {
            Serial.println(F("[nRF24] Empty payload ignored."));
            return;
        }
        const bool resumeRx = (currentMode == RadioMode::Receive);
        (void)sendOnePacket(payload, resumeRx);
        return;
    }
    if (line.startsWith("prefix ")) {
        String prefix = line.substring(7);
        prefix.trim();
        if (prefix.isEmpty()) {
            Serial.println(F("[nRF24] Prefix cannot be empty."));
            return;
        }
        burstPrefix = prefix;
        screenDirty = true;
        Serial.print(F("[nRF24] TX prefix updated to: "));
        Serial.println(burstPrefix);
        return;
    }
    if (line.startsWith("ch ")) {
        uint8_t newChannel = 0;
        if (!parseChannelValue(line.substring(3), newChannel)) {
            Serial.println(F("[nRF24] Unsupported channel. Use a value from 0 to 125."));
            return;
        }
        currentChannelValue = newChannel;
        for (uint8_t i = 0; i < kChannelChoiceCount; ++i) {
            if (kChannelChoices[i].channel == currentChannelValue) {
                currentChannelIndex = i;
                break;
            }
        }
        if (reinitializeRadioForCurrentMode()) {
            Serial.print(F("[nRF24] Channel switched to "));
            Serial.println(currentChannel());
        }
        return;
    }
    if (line.startsWith("rate ")) {
        uint8_t newIndex = 0;
        if (!parseDataRateToIndex(line.substring(5), newIndex)) {
            Serial.println(F("[nRF24] Unsupported rate. Use 250, 1000 or 2000."));
            return;
        }
        currentDataRateIndex = newIndex;
        if (reinitializeRadioForCurrentMode()) {
            Serial.print(F("[nRF24] Data rate switched to "));
            Serial.print(currentDataRateKbps());
            Serial.println(F(" kbps."));
        }
        return;
    }
    if (line.startsWith("power ")) {
        uint8_t newIndex = 0;
        if (!parsePowerToIndex(line.substring(6), newIndex)) {
            Serial.println(F("[nRF24] Unsupported power. Use -18, -12, -6 or 0."));
            return;
        }
        currentPowerIndex = newIndex;
        if (reinitializeRadioForCurrentMode()) {
            Serial.print(F("[nRF24] Power switched to "));
            Serial.print(currentOutputPowerDbm());
            Serial.println(F(" dBm."));
        }
        return;
    }
    if (line.startsWith("ack ")) {
        bool newAck = autoAckEnabled;
        if (!parseAckSetting(line.substring(4), newAck)) {
            Serial.println(F("[nRF24] Unsupported ACK value. Use on or off."));
            return;
        }
        autoAckEnabled = newAck;
        if (reinitializeRadioForCurrentMode()) {
            Serial.print(F("[nRF24] Auto ACK "));
            Serial.println(autoAckEnabled ? F("enabled.") : F("disabled."));
        }
        return;
    }

    Serial.print(F("[nRF24] Unknown command: "));
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

void handleBurstTransmit()
{
    if (currentMode != RadioMode::BurstTransmit) {
        return;
    }

    const unsigned long now = millis();
    if (lastBurstAtMs != 0U && (now - lastBurstAtMs) < kBurstIntervalMs) {
        return;
    }

    lastBurstAtMs = now;
    const String payload = burstPrefix + " #" + String(burstCounter++);
    (void)sendOnePacket(payload, false);
    screenDirty = true;
}

void cycleChannel()
{
    currentChannelIndex = static_cast<uint8_t>((currentChannelIndex + 1U) % kChannelChoiceCount);
    currentChannelValue = kChannelChoices[currentChannelIndex].channel;
    Serial.print(F("[nRF24] USER key -> channel "));
    Serial.println(currentChannel());
    (void)reinitializeRadioForCurrentMode();
}

void toggleMode()
{
    if (currentMode == RadioMode::Receive) {
        enterBurstTransmitMode();
    } else {
        (void)enterReceiveMode();
    }
}
}  // namespace

void init()
{
    currentMode = RadioMode::Receive;
    currentChannelIndex = 2;
    currentChannelValue = 76;
    currentDataRateIndex = 1;
    currentPowerIndex = 3;
    autoAckEnabled = true;
    lastBurstAtMs = 0;
    burstCounter = 0;
    rxCounter = 0;
    burstPrefix = kDefaultTxPrefix;
    lastRxPayload = "";
    lastRxLength = 0;
    hasLastRx = false;
    radioReady = false;
    screenDirty = true;
    backFocused = false;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;

    ledEffect = LedEffect::None;
    ledEffectUntilMs = 0;
    ledDirty = true;
    strip.begin();
    strip.setBrightness(255);
    setStripColor(0, 0, 0);

    radioReady = initRadio();
    if (!radioReady) {
        Serial.println(F("[nRF24] Radio init failed."));
    } else if (!enterReceiveMode()) {
        Serial.println(F("[nRF24] Failed to enter RX mode."));
        radioReady = false;
    }

    printStatus();
    printHelp();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    pollSerialCommands();

    if (takeUserButton() && !backFocused) {
        cycleChannel();
    }

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        toggleMode();
    }

    if (radioReady) {
        handleReceivedPacket();
        handleBurstTransmit();
    }

    checkLedTimeout();
    updateLeds();
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
    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.standby();
    pinMode(BOARD_NRF24_CE, OUTPUT);
    digitalWrite(BOARD_NRF24_CE, LOW);
    setStripColor(0, 0, 0);
    board_spi_deselect_all();
}

bool probeHardware(String& reason)
{
    reason = "";
    const bool ok = initRadio();

    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.sleep();
    pinMode(BOARD_NRF24_CE, OUTPUT);
    digitalWrite(BOARD_NRF24_CE, LOW);
    board_spi_deselect_all();

    if (!ok) {
        reason = "Optional module missing or SPI comm failed";
    }
    return ok;
}

}  // namespace page_nrf24
