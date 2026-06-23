#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <TFT_eSPI.h>

#include "utilities.h"

namespace {

constexpr uint8_t kRotation = 3;
constexpr uint8_t kUidBufferLen = 7;
constexpr uint8_t kPn532I2cAddress = BOARD_I2C_ADDR_1;
constexpr uint32_t kCardLostTimeoutMs = 700;
constexpr uint32_t kNoCardMessageMs = 1200;
constexpr uint32_t kRearmIntervalMs = 30;
constexpr uint32_t kBusSettleMs = 10;
constexpr uint32_t kAckTimeoutMs = 120;
constexpr uint32_t kResponseTimeoutMs = 120;

constexpr int16_t kUiMargin = 8;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;

constexpr int16_t kStatusX = 8;
constexpr int16_t kStatusY = 34;
constexpr int16_t kStatusW = 304;
constexpr int16_t kStatusH = 38;

constexpr int16_t kUidX = 8;
constexpr int16_t kUidY = 80;
constexpr int16_t kUidW = 304;
constexpr int16_t kUidH = 20;

constexpr int16_t kMetaY = 108;
constexpr int16_t kMetaW = 148;
constexpr int16_t kMetaH = 18;
constexpr int16_t kMetaLeftX = 8;
constexpr int16_t kMetaRightX = 164;

constexpr int16_t kSizeX = 8;
constexpr int16_t kSizeY = 134;
constexpr int16_t kSizeW = 304;
constexpr int16_t kSizeH = 18;

constexpr uint16_t kColorBg = 0x0841;
constexpr uint16_t kColorPanel = 0x1082;
constexpr uint16_t kColorPanelEdge = 0x31A6;
constexpr uint16_t kColorCard = 0x18C3;
constexpr uint16_t kColorPassBg = 0x0A41;
constexpr uint16_t kColorWarnBg = 0x5A00;
constexpr uint16_t kColorFailBg = 0x3006;

enum class UiState : uint8_t {
    Init = 0,
    Scanning,
    CardFound,
    ReadFail,
    NoCard,
    FatalError,
};

struct CardSnapshot {
    String uid;
    String type;
    String cardId;
    uint8_t uidLength = 0;

    void clear()
    {
        uid = "";
        type = "";
        cardId = "";
        uidLength = 0;
    }
};

Adafruit_PN532 nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);
TFT_eSPI tft;
TFT_eSprite canvas(&tft);

UiState uiState = UiState::Init;
CardSnapshot currentCard;
String detailLine;

bool screenDirty = true;
bool canvasReady = false;
bool readerReady = false;
bool detectionArmed = false;
bool cardPresent = false;

uint32_t readerVersionData = 0;
unsigned long lastSeenAtMs = 0;
unsigned long lastArmAttemptAtMs = 0;
unsigned long stateChangedAtMs = 0;

void setState(UiState next, const String &detail = String())
{
    if ((uiState != next) || (detailLine != detail)) {
        uiState = next;
        detailLine = detail;
        stateChangedAtMs = millis();
        screenDirty = true;
    }
}

String stateLabel()
{
    switch (uiState) {
    case UiState::Init:
        return "INIT";
    case UiState::Scanning:
        return "SCANNING";
    case UiState::CardFound:
        return "CARD FOUND";
    case UiState::ReadFail:
        return "READ FAIL";
    case UiState::NoCard:
        return "NO CARD";
    case UiState::FatalError:
        return "FATAL ERROR";
    }
    return "?";
}

uint16_t stateColor()
{
    switch (uiState) {
    case UiState::Init:
        return TFT_CYAN;
    case UiState::Scanning:
        return TFT_YELLOW;
    case UiState::CardFound:
        return TFT_GREEN;
    case UiState::ReadFail:
        return TFT_ORANGE;
    case UiState::NoCard:
        return TFT_DARKGREY;
    case UiState::FatalError:
        return TFT_RED;
    }
    return TFT_WHITE;
}

String firmwareVersionText()
{
    if (readerVersionData == 0) {
        return "-";
    }

    String text = String((readerVersionData >> 16) & 0xFF, DEC);
    text += ".";
    text += String((readerVersionData >> 8) & 0xFF, DEC);
    return text;
}

String uidToHexString(const uint8_t *uid, uint8_t uidLength)
{
    String text;
    text.reserve(uidLength * 3);

    for (uint8_t i = 0; i < uidLength; ++i) {
        if (i != 0) {
            text += ' ';
        }
        if (uid[i] < 0x10) {
            text += '0';
        }
        text += String(uid[i], HEX);
    }

    text.toUpperCase();
    return text;
}

String makeCardIdString(const uint8_t *uid, uint8_t uidLength)
{
    if (uidLength != 4) {
        return "-";
    }

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
    return String(buffer);
}

String guessCardType(uint8_t uidLength)
{
    if (uidLength == 4) {
        return "ISO14443A 4B UID";
    }
    if (uidLength == 7) {
        return "ISO14443A 7B UID";
    }
    return "ISO14443A";
}

void logCard(const CardSnapshot &card)
{
    Serial.print(F("[PN532] UID: "));
    Serial.println(card.uid);
    Serial.print(F("[PN532] Type: "));
    Serial.println(card.type);
    Serial.print(F("[PN532] UID Length: "));
    Serial.println(card.uidLength);
    Serial.print(F("[PN532] Card ID: "));
    Serial.println(card.cardId);
}

void cacheCard(const uint8_t *uid, uint8_t uidLength)
{
    currentCard.uid = uidToHexString(uid, uidLength);
    currentCard.type = guessCardType(uidLength);
    currentCard.cardId = makeCardIdString(uid, uidLength);
    currentCard.uidLength = uidLength;
}

void deselectSharedSpiDevices()
{
    pinMode(DISPLAY_CS, OUTPUT);
    digitalWrite(DISPLAY_CS, HIGH);

    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);

    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);
}

bool initDisplay()
{
    deselectSharedSpiDevices();

    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    tft.begin();
    tft.setRotation(kRotation);
    tft.fillScreen(TFT_BLACK);

    canvas.setColorDepth(16);
    canvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!canvasReady) {
        Serial.println(F("[PN532] Sprite allocation failed, using direct TFT redraw."));
    }

    digitalWrite(DISPLAY_BL, HIGH);
    return true;
}

bool initI2cBus()
{
    return Wire.setPins(BOARD_I2C_SDA, BOARD_I2C_SCL);
}

bool pn532WaitForIrq(uint32_t timeoutMs)
{
    const unsigned long startMs = millis();
    while (digitalRead(BOARD_PN532_IRQ) != LOW) {
        if (millis() - startMs > timeoutMs) {
            return false;
        }
        delay(1);
    }
    return true;
}

bool pn532ReadAckFrame()
{
    uint8_t ackFrame[7] = {0};
    size_t readCount = Wire.requestFrom(static_cast<uint8_t>(kPn532I2cAddress), static_cast<uint8_t>(sizeof(ackFrame)), static_cast<uint8_t>(true));
    if (readCount != sizeof(ackFrame)) {
        Serial.printf("[PN532] ACK read length mismatch: got %u\n", static_cast<unsigned>(readCount));
        return false;
    }

    for (uint8_t i = 0; i < sizeof(ackFrame); ++i) {
        ackFrame[i] = Wire.read();
    }

    static constexpr uint8_t expectedAck[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    if (ackFrame[0] != 0x01) {
        Serial.printf("[PN532] ACK ready byte mismatch: 0x%02X\n", ackFrame[0]);
        return false;
    }

    for (uint8_t i = 0; i < sizeof(expectedAck); ++i) {
        if (ackFrame[i + 1] != expectedAck[i]) {
            Serial.printf("[PN532] ACK payload mismatch at %u: 0x%02X\n", i, ackFrame[i + 1]);
            return false;
        }
    }
    return true;
}

bool pn532ReadShortResponse(uint8_t expectedResponseCode)
{
    uint8_t responseFrame[10] = {0};
    size_t readCount = Wire.requestFrom(static_cast<uint8_t>(kPn532I2cAddress),
                                        static_cast<uint8_t>(sizeof(responseFrame)),
                                        static_cast<uint8_t>(true));
    if (readCount != sizeof(responseFrame)) {
        Serial.printf("[PN532] Response read length mismatch: got %u\n", static_cast<unsigned>(readCount));
        return false;
    }

    for (uint8_t i = 0; i < sizeof(responseFrame); ++i) {
        responseFrame[i] = Wire.read();
    }

    if (responseFrame[0] != 0x01) {
        Serial.printf("[PN532] Response ready byte mismatch: 0x%02X\n", responseFrame[0]);
        return false;
    }
    if ((responseFrame[1] != 0x00) || (responseFrame[2] != 0x00) || (responseFrame[3] != 0xFF)) {
        Serial.println(F("[PN532] Response header mismatch."));
        return false;
    }
    if (responseFrame[6] != 0xD5) {
        Serial.printf("[PN532] Response TFI mismatch: 0x%02X\n", responseFrame[6]);
        return false;
    }
    if (responseFrame[7] != expectedResponseCode) {
        Serial.printf("[PN532] Unexpected response code: 0x%02X expected 0x%02X\n",
                      responseFrame[7], expectedResponseCode);
        return false;
    }
    return true;
}

bool pn532WriteCommandAckOnly(const uint8_t *cmd, uint8_t cmdLen)
{
    uint8_t packet[16] = {0};
    uint8_t len = cmdLen + 1;
    uint8_t sum = 0;

    packet[0] = 0x00;
    packet[1] = 0x00;
    packet[2] = 0xFF;
    packet[3] = len;
    packet[4] = static_cast<uint8_t>(~len + 1);
    packet[5] = 0xD4;

    for (uint8_t i = 0; i < cmdLen; ++i) {
        packet[6 + i] = cmd[i];
        sum += cmd[i];
    }
    packet[6 + cmdLen] = static_cast<uint8_t>(~(0xD4 + sum) + 1);
    packet[7 + cmdLen] = 0x00;

    Wire.beginTransmission(kPn532I2cAddress);
    Wire.write(packet, 8 + cmdLen);
    uint8_t txResult = Wire.endTransmission();
    if (txResult != 0) {
        Serial.printf("[PN532] Command write failed: %u\n", txResult);
        return false;
    }

    if (!pn532WaitForIrq(kAckTimeoutMs)) {
        Serial.println(F("[PN532] Timed out waiting for ACK IRQ."));
        return false;
    }

    return pn532ReadAckFrame();
}

bool pn532SendCommandWithShortResponse(const uint8_t *cmd,
                                       uint8_t cmdLen,
                                       uint8_t expectedResponseCode)
{
    if (!pn532WriteCommandAckOnly(cmd, cmdLen)) {
        return false;
    }

    if (!pn532WaitForIrq(kResponseTimeoutMs)) {
        Serial.println(F("[PN532] Timed out waiting for response IRQ."));
        return false;
    }

    return pn532ReadShortResponse(expectedResponseCode);
}

bool pn532ConfigurePassiveActivationRetries(uint8_t maxRetries)
{
    const uint8_t configCommand[] = {
        0x32,
        0x05,
        0xFF,
        0x01,
        maxRetries,
    };

    return pn532SendCommandWithShortResponse(configCommand,
                                             sizeof(configCommand),
                                             0x33);
}

bool armPassiveDetection()
{
    if (!readerReady || detectionArmed) {
        return readerReady;
    }

    static constexpr uint8_t detectCommand[] = {
        0x4A,
        0x01,
        PN532_MIFARE_ISO14443A,
    };

    lastArmAttemptAtMs = millis();
    if (!pn532WriteCommandAckOnly(detectCommand, sizeof(detectCommand))) {
        Serial.println(F("[PN532] Failed to arm passive target detection."));
        setState(UiState::ReadFail, "Failed to arm passive detection");
        return false;
    }

    detectionArmed = true;
    if (!cardPresent) {
        setState(UiState::Scanning, "Waiting for ISO14443A tag");
    }
    return true;
}

bool pn532IrqAsserted()
{
    return digitalRead(BOARD_PN532_IRQ) == LOW;
}

String statusDetailText();
const char *footerText();
uint16_t statusFillColor();
String uidText();
String typeText();
String uidLengthText();
String readerText();
String cardIdText();
String statusPillText();

template <typename Canvas>
void drawHeader(Canvas &gfx);

template <typename Canvas>
void drawFooter(Canvas &gfx);

template <typename Canvas>
void drawStatusPanel(Canvas &gfx);

template <typename Canvas>
void drawFieldCard(Canvas &gfx, int16_t x, int16_t y, int16_t w, int16_t h,
                   const char *label, const String &value, uint16_t borderColor);

template <typename Canvas>
void drawCardDetails(Canvas &gfx);

template <typename Canvas>
void drawUi(Canvas &gfx);

#include "pn532_test_ui.h"

void redrawScreen()
{
    if (canvasReady) {
        drawUi(canvas);
        canvas.pushSprite(0, 0);
    } else {
        drawUi(tft);
    }

    deselectSharedSpiDevices();
}

[[noreturn]] void showFatalError(const __FlashStringHelper *message)
{
    Serial.println(message);
    currentCard.clear();
    setState(UiState::FatalError, String(message));
    redrawScreen();

    while (true) {
        delay(1000);
    }
}

void handleDetectedTag(const uint8_t *uid, uint8_t uidLength)
{
    String detectedUid = uidToHexString(uid, uidLength);
    if (cardPresent && (detectedUid == currentCard.uid)) {
        lastSeenAtMs = millis();
        return;
    }

    cacheCard(uid, uidLength);
    cardPresent = true;
    lastSeenAtMs = millis();
    setState(UiState::CardFound, "ISO14443A tag detected");
    logCard(currentCard);
}

void handleCardTimeout()
{
    const unsigned long now = millis();

    if (cardPresent && (now - lastSeenAtMs > kCardLostTimeoutMs)) {
        cardPresent = false;
        currentCard.clear();
        setState(UiState::NoCard, "Card removed");
        Serial.println(F("[PN532] Card removed."));
        return;
    }

    if (!cardPresent) {
        if ((uiState == UiState::NoCard) && (now - stateChangedAtMs > kNoCardMessageMs)) {
            setState(UiState::Scanning, "Waiting for ISO14443A tag");
        } else if ((uiState == UiState::ReadFail) && (now - stateChangedAtMs > kNoCardMessageMs)) {
            setState(UiState::Scanning, "Waiting for ISO14443A tag");
        }
    }
}

void pollNfc()
{
    const unsigned long now = millis();
    if (!readerReady) {
        return;
    }

    if (!detectionArmed && (now - lastArmAttemptAtMs >= kRearmIntervalMs)) {
        armPassiveDetection();
    }

    if (detectionArmed && pn532IrqAsserted()) {
        uint8_t uid[kUidBufferLen] = {0};
        uint8_t uidLength = 0;

        detectionArmed = false;
        if (nfc.readDetectedPassiveTargetID(uid, &uidLength) && (uidLength > 0)) {
            handleDetectedTag(uid, uidLength);
        } else {
            Serial.println(F("[PN532] IRQ asserted but no valid tag payload was returned."));
            if (!cardPresent) {
                setState(UiState::ReadFail, "IRQ asserted but tag read failed");
            }
        }

        delay(kBusSettleMs);
        armPassiveDetection();
    }

    handleCardTimeout();
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println(F("T-Embed PN532 NFC screen test"));

    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    pinMode(BOARD_PN532_IRQ, INPUT_PULLUP);

    if (!initDisplay()) {
        showFatalError(F("[PN532] Display init failed."));
    }

    setState(UiState::Init, "Display ready");
    redrawScreen();

    if (!initI2cBus()) {
        showFatalError(F("[PN532] Failed to configure I2C pins."));
    }

    if (!nfc.begin()) {
        showFatalError(F("[PN532] begin() failed."));
    }

    Wire.setClock(100000U);
    Wire.setTimeOut(20);

    readerVersionData = nfc.getFirmwareVersion();
    if (!readerVersionData) {
        showFatalError(F("[PN532] Didn't find PN53x board."));
    }

    Serial.print(F("[PN532] Found chip PN5"));
    Serial.println((readerVersionData >> 24) & 0xFF, HEX);
    Serial.print(F("[PN532] Firmware ver. "));
    Serial.print((readerVersionData >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((readerVersionData >> 8) & 0xFF, DEC);

    if (!nfc.SAMConfig()) {
        showFatalError(F("[PN532] SAMConfig failed."));
    }

    if (!pn532ConfigurePassiveActivationRetries(0xFF)) {
        showFatalError(F("[PN532] Failed to set passive activation retries."));
    }

    readerReady = true;
    setState(UiState::Scanning, "IRQ-driven passive detection ready");
    redrawScreen();

    if (!armPassiveDetection()) {
        showFatalError(F("[PN532] Failed to start passive detection."));
    }
}

void loop()
{
    pollNfc();

    if (screenDirty) {
        screenDirty = false;
        redrawScreen();
    }

    delay(5);
}
