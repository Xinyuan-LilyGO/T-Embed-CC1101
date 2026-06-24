#pragma once

namespace page_sd {

namespace {
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
};

constexpr uint32_t kSdPowerSettleMs = 120;
constexpr uint32_t kSdMountFrequencies[] = {10000000UL, 4000000UL, 1000000UL};
constexpr uint32_t kFrameMs = 80;

constexpr int16_t kUiMargin = 8;
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

SdTestSummary summary;
bool screenDirty = true;
bool backFocused = false;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;

SPIClass& sharedSpi()
{
    return tft.getSPIinstance();
}

void resetSummaryForTest()
{
    summary = SdTestSummary();
    screenDirty = true;
}

String cardTypeStr(const uint8_t type)
{
    switch (type) {
        case CARD_MMC:  return "MMC";
        case CARD_SD:   return "SD";
        case CARD_SDHC: return "SDHC";
        default:        return "UNKNOWN";
    }
}

const char* stepStateText(const StepState state)
{
    switch (state) {
        case StepState::Pass: return "PASS";
        case StepState::Fail: return "FAIL";
        case StepState::Pending:
        default:              return "WAIT";
    }
}

uint16_t stepStateColor(const StepState state)
{
    switch (state) {
        case StepState::Pass: return TFT_GREEN;
        case StepState::Fail: return TFT_RED;
        case StepState::Pending:
        default:              return TFT_DARKGREY;
    }
}

uint16_t statusAccentColor()
{
    switch (summary.result) {
        case StepState::Pass: return TFT_GREEN;
        case StepState::Fail: return TFT_RED;
        case StepState::Pending:
        default:              return TFT_YELLOW;
    }
}

uint16_t statusFillColor()
{
    switch (summary.result) {
        case StepState::Pass: return kColorPassBg;
        case StepState::Fail: return kColorFailBg;
        case StepState::Pending:
        default:              return kColorPanel;
    }
}

String mountFrequencyText()
{
    if (!summary.mountedFrequency) {
        return "-";
    }
    return String(summary.mountedFrequency / 1000000UL) + " MHz";
}

String usageText()
{
    if (!summary.hasCardDetails) {
        return "-";
    }
    return String(static_cast<uint32_t>(summary.usedMB)) + " / " +
           String(static_cast<uint32_t>(summary.totalMB)) + " MB";
}

String rootEntriesText()
{
    return summary.rootMeasured ? String(summary.rootCount) + " entries" : String("-");
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

bool mountSdCard(uint32_t& mountedFrequency)
{
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    delay(kSdPowerSettleMs);

    for (const uint32_t frequency : kSdMountFrequencies) {
        SD.end();
        board_spi_deselect_all();
        delay(20);

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
    resetSummaryForTest();
    board_spi_deselect_all();

    uint32_t mountedFrequency = 0;
    if (!mountSdCard(mountedFrequency)) {
        summary.mount = StepState::Fail;
        summary.result = StepState::Fail;
        summary.title = "SD card mount failed";
        summary.detail = "Unable to mount the card at 10, 4 or 1 MHz.";

        Serial.println(F("[SD] SD.begin() failed."));
        Serial.println(F("[SD] TEST FAIL: mount failed."));
        screenDirty = true;
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

    const char* testPath = "/t_embed_sd_test.txt";
    const char* testData = "T-Embed SD test OK\n";
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
        Serial.println(F("[SD] TEST PASS."));
    } else {
        summary.result = StepState::Fail;
        summary.title = "SD read/write check failed";
        summary.detail = writeOk ? "Readback mismatch or open-for-read failed."
                                 : "Could not create the probe file on the card.";
        Serial.println(F("[SD] TEST FAIL: write/read check failed."));
    }

    Serial.println(F("[SD] Test complete."));
    screenDirty = true;
}

void drawHeader()
{
    tft.fillRect(0, 0, tft.width(), kHeaderH, TFT_NAVY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawString("SD Card Test", kUiMargin, 5, 2);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.drawString("Shared SPI", tft.width() - kUiMargin, 7, 1);
    tft.setTextDatum(TL_DATUM);
}

void drawFooter()
{
    const int16_t y = tft.height() - kFooterH;
    tft.fillRect(0, y, tft.width(), kFooterH, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString(backFocused ? "BOOT back" : "USER/BOOT retest", kUiMargin, y + 4, 1);
    drawBackButton(backFocused);
}

void drawStatusPanel()
{
    const uint16_t accent = statusAccentColor();
    const uint16_t fill = statusFillColor();
    tft.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, fill);
    tft.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, accent);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(accent, fill);
    tft.drawString(summary.title, kStatusX + 10, kStatusY + 6, 2);

    tft.fillRoundRect(kStatusX + kStatusW - 58, kStatusY + 7, 46, 16, 6, accent);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK, accent);
    tft.drawString(stepStateText(summary.result), kStatusX + kStatusW - 35, kStatusY + 15, 1);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, fill);
    tft.drawString(summary.detail, kStatusX + 10, kStatusY + 24, 1);
}

void drawStepCard(const int16_t x, const char* label, const StepState state)
{
    const uint16_t accent = stepStateColor(state);
    tft.fillRoundRect(x, kStepY, kStepW, kStepH, 6, kColorCard);
    tft.drawRoundRect(x, kStepY, kStepW, kStepH, 6, accent);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, kColorCard);
    tft.drawString(label, x + 6, kStepY + 5, 1);

    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(accent, kColorCard);
    tft.drawString(stepStateText(state), x + kStepW - 6, kStepY + 5, 1);
    tft.setTextDatum(TL_DATUM);
}

void drawMetricCard(const int16_t x, const int16_t y,
                    const char* label, const String& value, const uint16_t borderColor)
{
    tft.fillRoundRect(x, y, kMetricW, kMetricH, 6, kColorCard);
    tft.drawRoundRect(x, y, kMetricW, kMetricH, 6, borderColor);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_CYAN, kColorCard);
    tft.drawString(label, x + 8, y + 5, 1);

    tft.setTextColor(TFT_WHITE, kColorCard);
    tft.drawString(value, x + 52, y + 5, 1);
}

void redrawAll()
{
    board_prepare_display();
    tft.fillRect(0, 0, tft.width(), tft.height(), kColorBg);
    drawHeader();
    drawStatusPanel();

    drawStepCard(kUiMargin, "Mount", summary.mount);
    drawStepCard(kUiMargin + (kStepW + kStepGap), "Write", summary.write);
    drawStepCard(kUiMargin + (kStepW + kStepGap) * 2, "Read", summary.read);
    drawStepCard(kUiMargin + (kStepW + kStepGap) * 3, "Result", summary.result);

    drawMetricCard(kMetricLeftX, kMetricTopY, "SPI", mountFrequencyText(), kColorPanelEdge);
    drawMetricCard(kMetricRightX, kMetricTopY, "Type", summary.hasCardDetails ? cardTypeStr(summary.cardType) : String("-"), kColorPanelEdge);
    drawMetricCard(kMetricLeftX, kMetricBottomY, "Usage", usageText(), kColorPanelEdge);
    drawMetricCard(kMetricRightX, kMetricBottomY, "Root", rootEntriesText(), kColorPanelEdge);

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
    runSdTest();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    const bool userPressed = takeUserButton();
    const bool bootPressed = takeEncoderButton();

    if (bootPressed) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        Serial.println(F("[SD] BOOT key pressed, restarting test."));
        runSdTest();
        return;
    }

    if (userPressed && !backFocused) {
        Serial.println(F("[SD] USER key pressed, restarting test."));
        runSdTest();
    }
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
    SD.end();
    board_spi_deselect_all();
}

}  // namespace page_sd
