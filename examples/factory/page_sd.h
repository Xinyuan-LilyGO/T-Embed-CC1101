#pragma once

namespace page_sd {

namespace {
enum class StepState : uint8_t {
    Pending = 0,
    Pass,
    Fail,
};

constexpr uint32_t kFreqs[] = {10000000UL, 4000000UL, 1000000UL};
constexpr uint32_t kFrameMs = 80;

StepState mountState = StepState::Pending;
StepState writeState = StepState::Pending;
StepState readState = StepState::Pending;
bool backFocused = false;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;
uint32_t totalMB = 0;
uint32_t usedMB = 0;
uint32_t mountHz = 0;
uint16_t rootCount = 0;
String cardType = "-";
String detail = "Ready";

const char* stepText(const StepState s)
{
    switch (s) {
        case StepState::Pass: return "PASS";
        case StepState::Fail: return "FAIL";
        case StepState::Pending:
        default:              return "WAIT";
    }
}

uint16_t stepColor(const StepState s)
{
    switch (s) {
        case StepState::Pass: return TFT_GREEN;
        case StepState::Fail: return TFT_RED;
        case StepState::Pending:
        default:              return TFT_YELLOW;
    }
}

String cardTypeText(const uint8_t type)
{
    switch (type) {
        case CARD_MMC:  return "MMC";
        case CARD_SD:   return "SD";
        case CARD_SDHC: return "SDHC";
        default:        return "UNKNOWN";
    }
}

uint16_t listRootEntries()
{
    File root = SD.open("/");
    if (!root) {
        return 0;
    }

    uint16_t count = 0;
    File entry = root.openNextFile();
    while (entry) {
        entry.close();
        entry = root.openNextFile();
        ++count;
    }
    root.close();
    return count;
}

void runTest()
{
    mountState = StepState::Pending;
    writeState = StepState::Pending;
    readState = StepState::Pending;
    totalMB = 0;
    usedMB = 0;
    mountHz = 0;
    rootCount = 0;
    cardType = "-";
    detail = "Mounting SD card";
    screenDirty = true;

    SD.end();
    bool mounted = false;
    for (const uint32_t freq : kFreqs) {
        board_spi_deselect_all();
        delay(20);
        if (SD.begin(BOARD_SD_CS, tft.getSPIinstance(), freq)) {
            mounted = true;
            mountHz = freq;
            break;
        }
    }

    if (!mounted) {
        mountState = StepState::Fail;
        detail = "SD.begin failed";
        Serial.println(F("[SD] Mount failed."));
        return;
    }

    mountState = StepState::Pass;
    cardType = cardTypeText(SD.cardType());
    totalMB = static_cast<uint32_t>(SD.totalBytes() / (1024ULL * 1024ULL));
    usedMB = static_cast<uint32_t>(SD.usedBytes() / (1024ULL * 1024ULL));

    const char* path = "/factory_sd_test.txt";
    const char* data = "factory sd ok\n";

    File file = SD.open(path, FILE_WRITE);
    if (file) {
        writeState = file.print(data) == strlen(data) ? StepState::Pass : StepState::Fail;
        file.close();
    } else {
        writeState = StepState::Fail;
    }

    file = SD.open(path, FILE_READ);
    if (file) {
        const String line = file.readStringUntil('\n');
        readState = line.startsWith("factory sd ok") ? StepState::Pass : StepState::Fail;
        file.close();
    } else {
        readState = StepState::Fail;
    }

    SD.remove(path);
    rootCount = listRootEntries();
    detail = (mountState == StepState::Pass && writeState == StepState::Pass && readState == StepState::Pass)
        ? "SD self-check passed"
        : "SD read/write failed";
    Serial.println(String("[SD] ") + detail);
    screenDirty = true;
}
}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;
    runTest();
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
        runTest();
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
    drawPageHeader("SD Card", TFT_CYAN);
    drawPageFooter("BOOT=retest", backFocused);
    drawCard(8, 34, 304, 40, detail.c_str(), mountHz ? String(mountHz / 1000000UL) + " MHz" : "No mount", TFT_WHITE);
    drawCard(8, 82, 96, 30, "Mount", stepText(mountState), stepColor(mountState));
    drawCard(112, 82, 96, 30, "Write", stepText(writeState), stepColor(writeState));
    drawCard(216, 82, 96, 30, "Read", stepText(readState), stepColor(readState));
    drawCard(8, 120, 96, 30, "Type", cardType, TFT_YELLOW);
    drawCard(112, 120, 96, 30, "Usage", String(usedMB) + "/" + String(totalMB), TFT_GREEN);
    drawCard(216, 120, 96, 30, "Root", String(rootCount), TFT_CYAN);

    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    SD.end();
    board_spi_deselect_all();
}

}  // namespace page_sd
