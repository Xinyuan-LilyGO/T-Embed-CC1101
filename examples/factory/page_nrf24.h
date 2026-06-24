#pragma once

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
constexpr uint32_t kFrameMs = 80;

SPIClass& radioSpi = SPI;
nRF24 radio = new Module(BOARD_NRF24_CS, BOARD_NRF24_IRQ, BOARD_NRF24_CE, RADIOLIB_NC, radioSpi);

RadioMode currentMode = RadioMode::Receive;
uint8_t currentChannelIndex = 2;
uint8_t currentChannelValue = 76;
uint8_t currentDataRateIndex = 1;
uint8_t currentPowerIndex = 3;
bool autoAckEnabled = true;
bool radioReady = false;
bool backFocused = false;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;

unsigned long lastBurstAtMs = 0;
uint32_t burstCounter = 0;
uint32_t rxCounter = 0;
String burstPrefix = kDefaultTxPrefix;
String lastRxPayload = "-";
size_t lastRxLength = 0;
bool hasLastRx = false;

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

const char* modeLabel(const RadioMode mode)
{
    switch (mode) {
        case RadioMode::Receive:       return "RX";
        case RadioMode::BurstTransmit: return "TX BURST";
        default:                       return "?";
    }
}

float currentFrequencyMHz()
{
    return 2400.0f + static_cast<float>(currentChannel());
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
        Serial.printf("[nRF24] radio.begin failed: %d\n", state);
        return false;
    }

    state = radio.setAddressWidth(kAddressWidth);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setAddressWidth failed: %d\n", state);
        return false;
    }

    state = radio.setFrequency(currentFrequencyMHz());
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setFrequency failed: %d\n", state);
        return false;
    }

    state = radio.setBitRate(currentDataRateKbps());
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setBitRate failed: %d\n", state);
        return false;
    }

    state = radio.setOutputPower(currentOutputPowerDbm());
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setOutputPower failed: %d\n", state);
        return false;
    }

    state = radio.setAutoAck(autoAckEnabled);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setAutoAck failed: %d\n", state);
        return false;
    }

    state = radio.setReceivePipe(0, kPipeAddress);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setReceivePipe failed: %d\n", state);
        return false;
    }

    state = radio.setTransmitPipe(kPipeAddress);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] setTransmitPipe failed: %d\n", state);
        return false;
    }

    return true;
}

bool initRadio()
{
    board_spi_deselect_all();
    delay(20);
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
        Serial.printf("[nRF24] startReceive failed: %d\n", state);
        return false;
    }

    Serial.println(F("[nRF24] Mode -> RX"));
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

    Serial.println(F("[nRF24] Mode -> TX burst"));
}

bool transmitBlocking(const String& payload)
{
    board_spi_deselect_all();
    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.standby();
    (void)radio.setTransmitPipe(kPipeAddress);

    const int state = radio.startTransmit(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), 0);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[nRF24] startTransmit failed: %d\n", state);
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
            Serial.println(String("[nRF24] TX ") + payload);
            (void)radio.finishTransmit();
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
        Serial.printf("[nRF24] Invalid RX length: %u\n", static_cast<unsigned>(lastRxLength));
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
        Serial.println(String("[nRF24] RX ") + payload);
    } else {
        Serial.printf("[nRF24] RX failed: %d\n", state);
    }

    board_spi_deselect_all();
    (void)radio.startReceive();
}

void handleBurstTransmit()
{
    if (currentMode != RadioMode::BurstTransmit) {
        return;
    }

    const uint32_t now = millis();
    if (lastBurstAtMs != 0U && (now - lastBurstAtMs) < kBurstIntervalMs) {
        return;
    }

    lastBurstAtMs = now;
    const String payload = burstPrefix + String(" #") + String(burstCounter++);
    (void)sendOnePacket(payload, false);
    screenDirty = true;
}

void cycleChannel()
{
    currentChannelIndex = static_cast<uint8_t>((currentChannelIndex + 1U) % kChannelChoiceCount);
    currentChannelValue = kChannelChoices[currentChannelIndex].channel;
    Serial.print(F("[nRF24] USER channel -> "));
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

String clippedText(String text, const uint8_t maxLen)
{
    if (text.length() <= maxLen) {
        return text;
    }
    return text.substring(0, maxLen - 3) + "...";
}
}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;
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
    lastRxPayload = "-";
    lastRxLength = 0;
    hasLastRx = false;

    radioReady = initRadio();
    if (radioReady) {
        radioReady = enterReceiveMode();
    }
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    if (takeUserButton() && !backFocused) {
        cycleChannel();
    }

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        toggleMode();
        screenDirty = true;
    }

    if (!radioReady) {
        return;
    }

    if (currentMode == RadioMode::Receive) {
        handleReceivedPacket();
    } else {
        handleBurstTransmit();
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
    drawPageHeader("nRF24 Tx/Rx", TFT_CYAN);
    drawPageFooter("USER=chan  BOOT=mode", backFocused);
    drawStatusPill(radioReady ? "READY" : "FAIL", radioReady, 8, 34);

    const String countLabel = currentMode == RadioMode::Receive ? "RX count" : "TX count";
    const String countValue = currentMode == RadioMode::Receive ? String(rxCounter) : String(burstCounter);
    const String infoValue = currentMode == RadioMode::Receive
        ? (hasLastRx ? clippedText(lastRxPayload, 28) : String("Waiting..."))
        : clippedText(burstPrefix, 28);
    const String metaValue = currentMode == RadioMode::Receive
        ? (hasLastRx ? String(lastRxLength) + " bytes" : String("-"))
        : (String(kBurstIntervalMs) + " ms");

    drawCard(8, 62, 96, 42, "Channel", currentChannelLabel(), TFT_YELLOW);
    drawCard(112, 62, 96, 42, "Mode", modeLabel(currentMode), currentMode == RadioMode::Receive ? TFT_GREEN : TFT_ORANGE);
    drawCard(216, 62, 96, 42, countLabel.c_str(), countValue, TFT_WHITE);
    drawCard(8, 112, 200, 40, currentMode == RadioMode::Receive ? "Last RX" : "Prefix", infoValue, TFT_WHITE);
    drawCard(216, 112, 96, 40, currentMode == RadioMode::Receive ? "Length" : "Interval", metaValue, TFT_CYAN);

    tft.setTextColor(kUiMuted, kUiBg);
    tft.drawString(String(currentDataRateLabel()) + "  " + currentOutputPowerDbm() + " dBm  " +
                   (autoAckEnabled ? "ACK" : "NOACK"),
                   8, 156, 1);

    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    (void)radio.finishReceive();
    (void)radio.finishTransmit();
    (void)radio.standby();
    board_spi_deselect_all();
}

}  // namespace page_nrf24
