#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "utilities.h"

namespace {

constexpr uint8_t kRotation = 3;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr int16_t kUiMargin = 8;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;
constexpr uint32_t kBusSettleMs = 20;
constexpr uint32_t kSdPowerSettleMs = 120;
constexpr uint32_t kSdMountFrequencies[] = {10000000UL, 4000000UL, 1000000UL};

constexpr int16_t kStatusX = 8;
constexpr int16_t kStatusY = 34;
constexpr int16_t kStatusW = 304;
constexpr int16_t kStatusH = 38;

constexpr int16_t kStepY = 80;
constexpr int16_t kStepW = 72;
constexpr int16_t kStepH = 22;
constexpr int16_t kStepGap = 8;

constexpr int16_t kMetricTopY = 110;
constexpr int16_t kMetricBottomY = 132;
constexpr int16_t kMetricLeftX = 8;
constexpr int16_t kMetricRightX = 164;
constexpr int16_t kMetricW = 148;
constexpr int16_t kMetricH = 18;

constexpr uint16_t kColorBg = 0x0841;
constexpr uint16_t kColorPanel = 0x1082;
constexpr uint16_t kColorPanelEdge = 0x31A6;
constexpr uint16_t kColorCard = 0x18C3;
constexpr uint16_t kColorPassBg = 0x0A41;
constexpr uint16_t kColorFailBg = 0x3006;

enum class StepState : uint8_t {
    Pending = 0,
    Pass,
    Fail,
};

struct SdTestSummary {
    StepState mount = StepState::Pending;
    StepState write = StepState::Pending;
    StepState read = StepState::Pending;
    StepState result = StepState::Pending;

    uint32_t mountedFrequency = 0;
    uint8_t cardType = 0;
    bool hasCardDetails = false;
    uint64_t totalMB = 0;
    uint64_t usedMB = 0;
    uint16_t rootCount = 0;
    bool rootMeasured = false;

    String title = "Running SD diagnostics";
    String detail = "Checking mount, capacity and read/write path";
    String footer = "Testing...";
};

TFT_eSPI tft;
TFT_eSprite canvas(&tft);
bool canvasReady = false;
SdTestSummary summary;

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

String cardTypeStr(uint8_t type)
{
    switch (type) {
    case CARD_MMC:
        return "MMC";
    case CARD_SD:
        return "SD";
    case CARD_SDHC:
        return "SDHC";
    default:
        return "UNKNOWN";
    }
}

#include "test_sd_card_ui.h"

SPIClass &sharedSpi()
{
    return tft.getSPIinstance();
}

void resetSummaryForTest()
{
    summary = SdTestSummary();
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
}

uint16_t listRootToSerial()
{
    File root = SD.open("/");
    if (!root) {
        Serial.println(F("[SD] Failed to open root directory."));
        return 0;
    }

    Serial.println(F("[SD] Root directory:"));

    uint16_t count = 0;
    File entry = root.openNextFile();
    while (entry) {
        String name = String(entry.name());
        if (entry.isDirectory()) {
            name = "[" + name + "]";
        }

        Serial.print(F("  "));
        Serial.println(name);

        entry.close();
        entry = root.openNextFile();
        ++count;
    }

    if (!count) {
        Serial.println(F("  (empty)"));
    }

    root.close();
    return count;
}

bool mountSdCard(uint32_t &mountedFrequency)
{
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    delay(kSdPowerSettleMs);

    for (uint32_t frequency : kSdMountFrequencies) {
        SD.end();
        deselectSharedSpiDevices();
        delay(kBusSettleMs);

        Serial.print(F("[SD] Mount attempt @ "));
        Serial.print(frequency / 1000000UL);
        Serial.println(F(" MHz"));

        if (SD.begin(BOARD_SD_CS, sharedSpi(), frequency)) {
            mountedFrequency = frequency;
            return true;
        }
    }

    mountedFrequency = 0;
    return false;
}

void runSdTest()
{
    deselectSharedSpiDevices();

    uint32_t mountedFrequency = 0;
    if (!mountSdCard(mountedFrequency)) {
        summary.mount = StepState::Fail;
        summary.result = StepState::Fail;
        summary.title = "SD card mount failed";
        summary.detail = "Unable to mount the card at 10, 4 or 1 MHz.";
        summary.footer = "Press USER to retest";

        Serial.println(F("[SD] SD.begin() failed."));
        Serial.println(F("[SD] TEST FAIL: mount failed."));
        redrawScreen();
        return;
    }

    summary.mount = StepState::Pass;
    summary.mountedFrequency = mountedFrequency;
    summary.cardType = SD.cardType();
    summary.hasCardDetails = true;
    summary.totalMB = SD.totalBytes() / (1024ULL * 1024ULL);
    summary.usedMB = SD.usedBytes() / (1024ULL * 1024ULL);

    Serial.print(F("[SD] Card type: "));
    Serial.println(cardTypeStr(summary.cardType));
    Serial.printf("[SD] Total: %llu MB, Used: %llu MB\n", summary.totalMB, summary.usedMB);

    const char *testPath = "/t_embed_sd_test.txt";
    const char *testData = "T-Embed SD test OK\n";
    bool writeOk = false;
    bool readOk = false;

    File file = SD.open(testPath, FILE_WRITE);
    if (!file) {
        summary.write = StepState::Fail;
        Serial.println(F("[SD] Open for write failed."));
    } else {
        const size_t written = file.print(testData);
        file.close();

        writeOk = (written == strlen(testData));
        summary.write = writeOk ? StepState::Pass : StepState::Fail;
        Serial.println(writeOk ? F("[SD] Write OK.") : F("[SD] Write incomplete."));
    }

    file = SD.open(testPath, FILE_READ);
    if (!file) {
        summary.read = StepState::Fail;
        Serial.println(F("[SD] Open for read failed."));
    } else {
        const String line = file.readStringUntil('\n');
        file.close();

        if (line.startsWith("T-Embed SD test OK")) {
            readOk = true;
            summary.read = StepState::Pass;
            Serial.println(F("[SD] Read OK."));
        } else {
            summary.read = StepState::Fail;
            Serial.print(F("[SD] Read mismatch: "));
            Serial.println(line);
        }
    }

    SD.remove(testPath);

    summary.rootCount = listRootToSerial();
    summary.rootMeasured = true;

    if (writeOk && readOk) {
        summary.result = StepState::Pass;
        summary.title = "SD card self-check passed";
        summary.detail = "Mount, write/readback and directory probe completed.";
        summary.footer = "Press USER to retest";
        Serial.println(F("[SD] TEST PASS."));
    } else {
        summary.result = StepState::Fail;
        summary.title = "SD read/write check failed";
        summary.detail = writeOk ? "Readback mismatch or open-for-read failed."
                                 : "Could not create the probe file on the card.";
        summary.footer = "Press USER to retest";
        Serial.println(F("[SD] TEST FAIL: write/read check failed."));
    }

    Serial.println(F("[SD] Test complete."));
    redrawScreen();
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("T-Embed SD Card Test"));

    pinMode(BOARD_USER_KEY, INPUT_PULLUP);

    if (!initDisplayPower()) {
        Serial.println(F("[SD] Display power init failed - halting."));
        while (true) {
            delay(1000);
        }
    }

    canvas.setColorDepth(16);
    canvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!canvasReady) {
        Serial.println(F("[SD] Sprite allocation failed, using direct TFT redraw."));
    }

    redrawScreen();
    runSdTest();
}

void loop()
{
    if (boardUserKeyPressed()) {
        Serial.println(F("[SD] USER key pressed, restarting test."));
        resetSummaryForTest();
        redrawScreen();
        runSdTest();
    }

    delay(20);
}
