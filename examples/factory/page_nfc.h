#pragma once

namespace page_nfc {

namespace {
enum class UiState : uint8_t {
    Init = 0,
    Scanning,
    CardFound,
    ReadFail,
    NoCard,
    Error,
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

constexpr uint8_t kUidBufferLen = 7;
constexpr uint8_t kPn532I2cAddress = BOARD_I2C_ADDR_1;
constexpr uint32_t kCardLostTimeoutMs = 700;
constexpr uint32_t kNoCardMessageMs = 1200;
constexpr uint32_t kRearmIntervalMs = 30;
constexpr uint32_t kAckTimeoutMs = 120;
constexpr uint32_t kResponseTimeoutMs = 120;
constexpr uint32_t kFrameMs = 80;

Adafruit_PN532 nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);
UiState uiState = UiState::Init;
CardSnapshot currentCard;
String detailLine = "Initializing";
bool readerReady = false;
bool detectionArmed = false;
bool cardPresent = false;
bool backFocused = false;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;
uint32_t readerVersionData = 0;
unsigned long lastSeenAtMs = 0;
unsigned long lastArmAttemptAtMs = 0;
unsigned long stateChangedAtMs = 0;

void setState(const UiState next, const String& detail = String())
{
    if (uiState != next || detailLine != detail) {
        uiState = next;
        detailLine = detail;
        stateChangedAtMs = millis();
        screenDirty = true;
    }
}

const char* stateLabel()
{
    switch (uiState) {
        case UiState::Init:      return "INIT";
        case UiState::Scanning:  return "SCAN";
        case UiState::CardFound: return "FOUND";
        case UiState::ReadFail:  return "WARN";
        case UiState::NoCard:    return "IDLE";
        case UiState::Error:     return "ERROR";
        default:                 return "?";
    }
}

uint16_t stateColor()
{
    switch (uiState) {
        case UiState::Init:      return TFT_CYAN;
        case UiState::Scanning:  return TFT_YELLOW;
        case UiState::CardFound: return TFT_GREEN;
        case UiState::ReadFail:  return TFT_ORANGE;
        case UiState::NoCard:    return TFT_LIGHTGREY;
        case UiState::Error:     return TFT_RED;
        default:                 return TFT_WHITE;
    }
}

String firmwareVersionText()
{
    if (readerVersionData == 0) {
        return "-";
    }
    return String((readerVersionData >> 16) & 0xFF) + "." + String((readerVersionData >> 8) & 0xFF);
}

String uidToHexString(const uint8_t* uid, const uint8_t uidLength)
{
    String text;
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

String makeCardIdString(const uint8_t* uid, const uint8_t uidLength)
{
    if (uidLength != 4) {
        return "-";
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
    return String(buffer);
}

String guessCardType(const uint8_t uidLength)
{
    if (uidLength == 4) {
        return "ISO14443A 4B";
    }
    if (uidLength == 7) {
        return "ISO14443A 7B";
    }
    return "ISO14443A";
}

void cacheCard(const uint8_t* uid, const uint8_t uidLength)
{
    currentCard.uid = uidToHexString(uid, uidLength);
    currentCard.type = guessCardType(uidLength);
    currentCard.cardId = makeCardIdString(uid, uidLength);
    currentCard.uidLength = uidLength;
}

bool pn532WaitForIrq(const uint32_t timeoutMs)
{
    const unsigned long startedAtMs = millis();
    while (digitalRead(BOARD_PN532_IRQ) != LOW) {
        if ((millis() - startedAtMs) > timeoutMs) {
            return false;
        }
        delay(1);
    }
    return true;
}

bool pn532ReadAckFrame()
{
    uint8_t ackFrame[7] = {0};
    const size_t readCount = Wire.requestFrom(static_cast<uint8_t>(kPn532I2cAddress),
                                              static_cast<uint8_t>(sizeof(ackFrame)),
                                              static_cast<uint8_t>(true));
    if (readCount != sizeof(ackFrame)) {
        return false;
    }

    for (uint8_t i = 0; i < sizeof(ackFrame); ++i) {
        ackFrame[i] = Wire.read();
    }

    static constexpr uint8_t expectedAck[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    if (ackFrame[0] != 0x01) {
        return false;
    }
    for (uint8_t i = 0; i < sizeof(expectedAck); ++i) {
        if (ackFrame[i + 1] != expectedAck[i]) {
            return false;
        }
    }
    return true;
}

bool pn532ReadShortResponse(const uint8_t expectedResponseCode)
{
    uint8_t responseFrame[10] = {0};
    const size_t readCount = Wire.requestFrom(static_cast<uint8_t>(kPn532I2cAddress),
                                              static_cast<uint8_t>(sizeof(responseFrame)),
                                              static_cast<uint8_t>(true));
    if (readCount != sizeof(responseFrame)) {
        return false;
    }

    for (uint8_t i = 0; i < sizeof(responseFrame); ++i) {
        responseFrame[i] = Wire.read();
    }

    return responseFrame[0] == 0x01 &&
           responseFrame[1] == 0x00 &&
           responseFrame[2] == 0x00 &&
           responseFrame[3] == 0xFF &&
           responseFrame[6] == 0xD5 &&
           responseFrame[7] == expectedResponseCode;
}

bool pn532WriteCommandAckOnly(const uint8_t* cmd, const uint8_t cmdLen)
{
    uint8_t packet[16] = {0};
    const uint8_t len = cmdLen + 1;
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
    if (Wire.endTransmission() != 0) {
        return false;
    }

    return pn532WaitForIrq(kAckTimeoutMs) && pn532ReadAckFrame();
}

bool pn532SendCommandWithShortResponse(const uint8_t* cmd,
                                       const uint8_t cmdLen,
                                       const uint8_t expectedResponseCode)
{
    return pn532WriteCommandAckOnly(cmd, cmdLen) &&
           pn532WaitForIrq(kResponseTimeoutMs) &&
           pn532ReadShortResponse(expectedResponseCode);
}

bool pn532ConfigurePassiveActivationRetries(const uint8_t maxRetries)
{
    const uint8_t configCommand[] = {0x32, 0x05, 0xFF, 0x01, maxRetries};
    return pn532SendCommandWithShortResponse(configCommand, sizeof(configCommand), 0x33);
}

bool armPassiveDetection()
{
    if (!readerReady || detectionArmed) {
        return readerReady;
    }

    static constexpr uint8_t detectCommand[] = {0x4A, 0x01, PN532_MIFARE_ISO14443A};
    lastArmAttemptAtMs = millis();
    if (!pn532WriteCommandAckOnly(detectCommand, sizeof(detectCommand))) {
        setState(UiState::ReadFail, "Failed to arm passive detect");
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

void handleDetectedTag(const uint8_t* uid, const uint8_t uidLength)
{
    const String detectedUid = uidToHexString(uid, uidLength);
    if (cardPresent && detectedUid == currentCard.uid) {
        lastSeenAtMs = millis();
        return;
    }

    cacheCard(uid, uidLength);
    cardPresent = true;
    lastSeenAtMs = millis();
    setState(UiState::CardFound, "ISO14443A tag detected");
    Serial.println(String("[NFC] UID ") + currentCard.uid);
}

void handleCardTimeout()
{
    const unsigned long now = millis();
    if (cardPresent && (now - lastSeenAtMs > kCardLostTimeoutMs)) {
        cardPresent = false;
        currentCard.clear();
        setState(UiState::NoCard, "Card removed");
        return;
    }

    if (!cardPresent && (uiState == UiState::NoCard || uiState == UiState::ReadFail) &&
        (now - stateChangedAtMs > kNoCardMessageMs)) {
        setState(UiState::Scanning, "Waiting for ISO14443A tag");
    }
}

bool initReader()
{
    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    pinMode(BOARD_PN532_IRQ, INPUT_PULLUP);

    currentCard.clear();
    readerReady = false;
    detectionArmed = false;
    cardPresent = false;
    readerVersionData = 0;
    lastSeenAtMs = 0;
    lastArmAttemptAtMs = 0;
    setState(UiState::Init, "Starting PN532");

    Wire.setClock(100000U);
    Wire.setTimeOut(20);

    if (!nfc.begin()) {
        setState(UiState::Error, "PN532 begin failed");
        return false;
    }

    readerVersionData = nfc.getFirmwareVersion();
    if (!readerVersionData) {
        setState(UiState::Error, "PN532 firmware missing");
        return false;
    }

    if (!nfc.SAMConfig()) {
        setState(UiState::Error, "SAMConfig failed");
        return false;
    }

    if (!pn532ConfigurePassiveActivationRetries(0xFF)) {
        setState(UiState::Error, "Passive retry config failed");
        return false;
    }

    readerReady = true;
    return armPassiveDetection();
}

void pollNfc()
{
    const unsigned long now = millis();
    if (!readerReady) {
        return;
    }

    if (!detectionArmed && (now - lastArmAttemptAtMs) >= kRearmIntervalMs) {
        (void)armPassiveDetection();
    }

    if (detectionArmed && pn532IrqAsserted()) {
        uint8_t uid[kUidBufferLen] = {0};
        uint8_t uidLength = 0;

        detectionArmed = false;
        if (nfc.readDetectedPassiveTargetID(uid, &uidLength) && uidLength > 0) {
            handleDetectedTag(uid, uidLength);
        } else if (!cardPresent) {
            setState(UiState::ReadFail, "IRQ asserted but read failed");
        }

        delay(10);
        (void)armPassiveDetection();
    }

    handleCardTimeout();
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
    (void)initReader();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    (void)takeUserButton();

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        (void)initReader();
    }

    pollNfc();
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    tft.fillScreen(kUiBg);
    drawPageHeader("PN532 NFC", stateColor());
    drawPageFooter("BOOT=reinit", backFocused);
    drawCard(8, 34, 304, 40, stateLabel(), clippedText(detailLine, 34), stateColor());
    drawCard(8, 82, 96, 30, "Reader", firmwareVersionText(), TFT_CYAN);
    drawCard(112, 82, 96, 30, "UID Len", currentCard.uidLength ? String(currentCard.uidLength) + "B" : "-", TFT_GREEN);
    drawCard(216, 82, 96, 30, "Card ID", currentCard.cardId.isEmpty() ? "-" : currentCard.cardId, TFT_YELLOW);
    drawCard(8, 120, 304, 30, "Last UID", currentCard.uid.isEmpty() ? "-" : currentCard.uid, TFT_WHITE);

    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit() {}

}  // namespace page_nfc
