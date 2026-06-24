#pragma once

namespace page_nfc {

namespace {
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

constexpr uint8_t kUidBufferLen = 7;
constexpr uint8_t kPn532I2cAddress = BOARD_I2C_ADDR_1;
constexpr uint32_t kCardLostTimeoutMs = 700;
constexpr uint32_t kNoCardMessageMs = 1200;
constexpr uint32_t kRearmIntervalMs = 30;
constexpr uint32_t kAckTimeoutMs = 120;
constexpr uint32_t kResponseTimeoutMs = 120;
constexpr uint32_t kFrameMs = 40;

constexpr int16_t kUiMargin = 8;
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
constexpr int16_t kSizeY = 134;

constexpr uint16_t kColorBg = 0x0841;
constexpr uint16_t kColorPanel = 0x1082;
constexpr uint16_t kColorPanelEdge = 0x31A6;
constexpr uint16_t kColorCard = 0x18C3;
constexpr uint16_t kColorPassBg = 0x0A41;
constexpr uint16_t kColorWarnBg = 0x5A00;
constexpr uint16_t kColorFailBg = 0x3006;

Adafruit_PN532 nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);
UiState uiState = UiState::Init;
CardSnapshot currentCard;
String detailLine = "Starting PN532";

bool screenDirty = true;
bool readerReady = false;
bool detectionArmed = false;
bool cardPresent = false;
bool backFocused = false;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;
uint32_t readerVersionData = 0;
unsigned long lastSeenAtMs = 0;
unsigned long lastArmAttemptAtMs = 0;
unsigned long stateChangedAtMs = 0;

void setState(const UiState next, const String& detail = String())
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
        case UiState::Init:       return "INIT";
        case UiState::Scanning:   return "SCANNING";
        case UiState::CardFound:  return "CARD FOUND";
        case UiState::ReadFail:   return "READ FAIL";
        case UiState::NoCard:     return "NO CARD";
        case UiState::FatalError: return "FATAL ERROR";
        default:                  return "?";
    }
}

uint16_t stateColor()
{
    switch (uiState) {
        case UiState::Init:       return TFT_CYAN;
        case UiState::Scanning:   return TFT_YELLOW;
        case UiState::CardFound:  return TFT_GREEN;
        case UiState::ReadFail:   return TFT_ORANGE;
        case UiState::NoCard:     return TFT_DARKGREY;
        case UiState::FatalError: return TFT_RED;
        default:                  return TFT_WHITE;
    }
}

uint16_t statusFillColor()
{
    switch (uiState) {
        case UiState::CardFound:  return kColorPassBg;
        case UiState::ReadFail:   return kColorWarnBg;
        case UiState::FatalError: return kColorFailBg;
        case UiState::NoCard:     return kColorCard;
        case UiState::Init:
        case UiState::Scanning:
        default:                  return kColorPanel;
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
        return "ISO14443A 4B UID";
    }
    if (uidLength == 7) {
        return "ISO14443A 7B UID";
    }
    return "ISO14443A";
}

void logCard(const CardSnapshot& card)
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

void cacheCard(const uint8_t* uid, const uint8_t uidLength)
{
    currentCard.uid = uidToHexString(uid, uidLength);
    currentCard.type = guessCardType(uidLength);
    currentCard.cardId = makeCardIdString(uid, uidLength);
    currentCard.uidLength = uidLength;
}

bool pn532WaitForIrq(const uint32_t timeoutMs)
{
    const unsigned long startMs = millis();
    while (digitalRead(BOARD_PN532_IRQ) != LOW) {
        if ((millis() - startMs) > timeoutMs) {
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

bool pn532ReadShortResponse(const uint8_t expectedResponseCode)
{
    uint8_t responseFrame[10] = {0};
    const size_t readCount = Wire.requestFrom(static_cast<uint8_t>(kPn532I2cAddress),
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
    if (responseFrame[1] != 0x00 || responseFrame[2] != 0x00 || responseFrame[3] != 0xFF) {
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
    const uint8_t txResult = Wire.endTransmission();
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

bool pn532SendCommandWithShortResponse(const uint8_t* cmd,
                                       const uint8_t cmdLen,
                                       const uint8_t expectedResponseCode)
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

    if (!cardPresent &&
        (uiState == UiState::NoCard || uiState == UiState::ReadFail) &&
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
        setState(UiState::FatalError, "begin() failed");
        return false;
    }

    readerVersionData = nfc.getFirmwareVersion();
    if (!readerVersionData) {
        setState(UiState::FatalError, "Didn't find PN53x board");
        return false;
    }

    if (!nfc.SAMConfig()) {
        setState(UiState::FatalError, "SAMConfig failed");
        return false;
    }

    if (!pn532ConfigurePassiveActivationRetries(0xFF)) {
        setState(UiState::FatalError, "Passive retry config failed");
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

    if (!detectionArmed && (now - lastArmAttemptAtMs >= kRearmIntervalMs)) {
        armPassiveDetection();
    }

    if (detectionArmed && pn532IrqAsserted()) {
        uint8_t uid[kUidBufferLen] = {0};
        uint8_t uidLength = 0;

        detectionArmed = false;
        if (nfc.readDetectedPassiveTargetID(uid, &uidLength) && uidLength > 0) {
            handleDetectedTag(uid, uidLength);
        } else if (!cardPresent) {
            Serial.println(F("[PN532] IRQ asserted but no valid tag payload was returned."));
            setState(UiState::ReadFail, "IRQ asserted but tag read failed");
        }

        delay(10);
        armPassiveDetection();
    }

    handleCardTimeout();
}

String statusDetailText()
{
    if (!detailLine.isEmpty()) {
        return detailLine;
    }

    switch (uiState) {
        case UiState::Init:       return "Bringing up display and PN532 reader";
        case UiState::Scanning:   return "Bring an ISO14443A tag close to the antenna";
        case UiState::CardFound:  return "Tag identified successfully";
        case UiState::ReadFail:   return "Tag event arrived, but payload read failed";
        case UiState::NoCard:     return "Tag removed";
        case UiState::FatalError: return "Check serial output for details";
        default:                  return "";
    }
}

String uidText()
{
    return currentCard.uid.isEmpty() ? String("-") : currentCard.uid;
}

String typeText()
{
    return currentCard.type.isEmpty() ? String("-") : currentCard.type;
}

String uidLengthText()
{
    return currentCard.uidLength == 0 ? String("-") : String(currentCard.uidLength) + " bytes";
}

String readerText()
{
    return readerVersionData == 0 ? String("-") : String("PN532 FW ") + firmwareVersionText();
}

String cardIdText()
{
    return currentCard.cardId.isEmpty() ? String("-") : currentCard.cardId;
}

String statusPillText()
{
    switch (uiState) {
        case UiState::Init:       return "BOOT";
        case UiState::Scanning:   return "SCAN";
        case UiState::CardFound:  return "LIVE";
        case UiState::ReadFail:   return "WARN";
        case UiState::NoCard:     return "IDLE";
        case UiState::FatalError: return "STOP";
        default:                  return "?";
    }
}

void drawHeader()
{
    tft.fillRect(0, 0, tft.width(), kHeaderH, TFT_NAVY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("PN532 NFC Test", kUiMargin, 5, 2);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.drawString("I2C / IRQ", tft.width() - kUiMargin, 7, 1);
    tft.setTextDatum(TL_DATUM);
}

void drawFooter()
{
    const int16_t y = tft.height() - kFooterH;
    tft.fillRect(0, y, tft.width(), kFooterH, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString(backFocused ? "BOOT back" : "BOOT reinit", 6, y + 4, 1);
    drawBackButton(backFocused);
}

void drawStatusPanel()
{
    const uint16_t accent = stateColor();
    const uint16_t fill = statusFillColor();

    tft.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, fill);
    tft.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, accent);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(accent, fill);
    tft.drawString(stateLabel(), kStatusX + 10, kStatusY + 6, 2);

    tft.fillRoundRect(kStatusX + kStatusW - 60, kStatusY + 7, 48, 16, 6, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, accent);
    tft.drawString(statusPillText(), kStatusX + kStatusW - 36, kStatusY + 15, 1);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, fill);
    tft.drawString(statusDetailText(), kStatusX + 10, kStatusY + 24, 1);
}

void drawFieldCard(const int16_t x, const int16_t y, const int16_t w, const int16_t h,
                   const char* label, const String& value, const uint16_t borderColor)
{
    tft.fillRoundRect(x, y, w, h, 6, kColorCard);
    tft.drawRoundRect(x, y, w, h, 6, borderColor);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, kColorCard);
    tft.drawString(label, x + 8, y + 5, 1);
    tft.setTextColor(TFT_WHITE, kColorCard);
    tft.drawString(value, x + 56, y + 5, 1);
}

void drawCardDetails()
{
    tft.fillRoundRect(kUidX, kUidY, kUidW, kUidH, 6, kColorCard);
    tft.drawRoundRect(kUidX, kUidY, kUidW, kUidH, 6, stateColor());
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, kColorCard);
    tft.drawString("UID", kUidX + 8, kUidY + 6, 1);
    tft.setTextColor(TFT_WHITE, kColorCard);
    tft.drawString(uidText(), kUidX + 56, kUidY + 6, 1);

    drawFieldCard(kMetaLeftX, kMetaY, kMetaW, kMetaH, "Type", typeText(), kColorPanelEdge);
    drawFieldCard(kMetaRightX, kMetaY, kMetaW, kMetaH, "UID Len", uidLengthText(), kColorPanelEdge);
    drawFieldCard(kMetaLeftX, kSizeY, kMetaW, kMetaH, "Reader", readerText(), kColorPanelEdge);
    drawFieldCard(kMetaRightX, kSizeY, kMetaW, kMetaH, "Card ID", cardIdText(),
                  currentCard.uidLength == 4 ? kColorPanelEdge : TFT_DARKGREY);
}

void redrawAll()
{
    board_prepare_display();
    tft.fillRect(0, 0, tft.width(), tft.height(), kColorBg);
    drawHeader();
    drawStatusPanel();
    drawCardDetails();
    drawFooter();
    board_spi_deselect_all();
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
    if (!screenDirty || (lastDrawMs != 0U && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    gSubPageGfx.beginFrame();
    redrawAll();
    gSubPageGfx.endFrame();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit() {}

}  // namespace page_nfc
