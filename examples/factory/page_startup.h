#pragma once

#include "factory_build_info.h"

namespace page_startup {

namespace {
enum class HardwareProfile : uint8_t {
    Unknown = 0,
    V10,
    V11,
    Mixed,
};

constexpr uint32_t kFrameMs = 80;
constexpr uint32_t kPn532WakeDelayMs = 12;
constexpr uint32_t kPn532ResetPulseMs = 2;

TFT_eSprite canvas(&tft);
bool canvasReady = false;
bool screenDirty = true;
bool enterMenuRequested = false;
bool allowMenuEntry = true;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;

bool hasAddr22 = false;
bool hasAddr24 = false;
bool hasAddr55 = false;
bool hasAddr6A = false;
bool hasAddr6B = false;
HardwareProfile detectedProfile = HardwareProfile::Unknown;

String clipText(const String& text, const uint8_t maxChars)
{
    if (text.length() <= maxChars || maxChars < 4) {
        return text;
    }
    return text.substring(0, maxChars - 3) + "...";
}

String hardwareVersionText()
{
    switch (detectedProfile) {
        case HardwareProfile::V10:
            return "T-Embed-CC1101 V1.0";
        case HardwareProfile::V11:
            return "T-Embed-CC1101 V1.1";
        case HardwareProfile::Mixed:
            return "Mixed / Unknown";
        case HardwareProfile::Unknown:
        default:
            return "Not identified";
    }
}

const char* supportBadgeText()
{
    switch (detectedProfile) {
        case HardwareProfile::V10:
            return "V1.0 OK";
        case HardwareProfile::V11:
            return "V1.1 BLOCK";
        case HardwareProfile::Mixed:
            return "CHECK HW";
        case HardwareProfile::Unknown:
        default:
            return "NO I2C";
    }
}

uint16_t supportBadgeColor()
{
    switch (detectedProfile) {
        case HardwareProfile::V10:
            return TFT_GREEN;
        case HardwareProfile::V11:
            return TFT_RED;
        case HardwareProfile::Mixed:
            return TFT_YELLOW;
        case HardwareProfile::Unknown:
        default:
            return TFT_RED;
    }
}

uint16_t supportBadgeBg()
{
    switch (detectedProfile) {
        case HardwareProfile::V10:
            return kPassBg;
        case HardwareProfile::V11:
            return kFailBg;
        case HardwareProfile::Mixed:
            return 0x6320;
        case HardwareProfile::Unknown:
        default:
            return kFailBg;
    }
}

String supportText()
{
    switch (detectedProfile) {
        case HardwareProfile::V10:
            return "This firmware matches V1.0 hardware.";
        case HardwareProfile::V11:
            return "V1.1 detected. Main menu is blocked.";
        case HardwareProfile::Mixed:
            return "Probe is mixed. Continue with caution.";
        case HardwareProfile::Unknown:
        default:
            return "No I2C devices found. Main menu is blocked.";
    }
}

String footerText()
{
    if (allowMenuEntry) {
        return "USER/BOOT/ENC to enter main menu";
    }
    if (detectedProfile == HardwareProfile::V11) {
        return "V1.1 blocked. Firmware is for V1.0.";
    }
    if (detectedProfile == HardwareProfile::Unknown) {
        return "No I2C found. Main menu is blocked.";
    }
    return "Hardware check required before menu entry.";
}

void preparePn532ForProbe()
{
    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, LOW);
    delay(kPn532ResetPulseMs);
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    pinMode(BOARD_PN532_IRQ, INPUT_PULLUP);
    Wire.setClock(100000U);
    Wire.setTimeOut(20);
    delay(kPn532WakeDelayMs);
}

bool probePn532Address()
{
    preparePn532ForProbe();

    if (i2cDevicePresent(0x24)) {
        return true;
    }

    delay(5);
    return i2cDevicePresent(0x24);
}

void detectHardwareProfile()
{
    hasAddr22 = i2cDevicePresent(0x22);
    hasAddr24 = probePn532Address();
    hasAddr55 = i2cDevicePresent(0x55);
    hasAddr6A = i2cDevicePresent(0x6A);
    hasAddr6B = i2cDevicePresent(0x6B);

    const bool looksV10 = hasAddr24 && hasAddr55 && hasAddr6B && !hasAddr22 && !hasAddr6A;
    const bool looksV11 = hasAddr22 && hasAddr55 && hasAddr6A && !hasAddr24 && !hasAddr6B;
    const bool anyKnown = hasAddr22 || hasAddr24 || hasAddr55 || hasAddr6A || hasAddr6B;

    if (looksV10) {
        detectedProfile = HardwareProfile::V10;
    } else if (looksV11) {
        detectedProfile = HardwareProfile::V11;
    } else if (anyKnown) {
        detectedProfile = HardwareProfile::Mixed;
    } else {
        detectedProfile = HardwareProfile::Unknown;
    }

    allowMenuEntry = (detectedProfile != HardwareProfile::V11) &&
                     (detectedProfile != HardwareProfile::Unknown);

    Serial.printf("[BOOT] I2C probe 22=%d 24=%d 55=%d 6A=%d 6B=%d\n",
                  hasAddr22 ? 1 : 0,
                  hasAddr24 ? 1 : 0,
                  hasAddr55 ? 1 : 0,
                  hasAddr6A ? 1 : 0,
                  hasAddr6B ? 1 : 0);
    Serial.printf("[BOOT] Hardware profile: %s  allowMenu=%d\n",
                  hardwareVersionText().c_str(),
                  allowMenuEntry ? 1 : 0);
}

template <typename Canvas>
void drawHeader(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderH, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("Factory Startup", kMargin, 5, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("T-Embed CC1101", gfx.width() - kMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas& gfx)
{
    const int16_t y = gfx.height() - kFooterH;
    gfx.fillRect(0, y, gfx.width(), kFooterH, TFT_DARKGREY);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(clipText(footerText(), 38), kMargin, y + 4, 1);
}

template <typename Canvas>
void drawStatusBadge(Canvas& gfx)
{
    const int16_t x = 8;
    const int16_t y = 34;
    const int16_t w = 92;
    const int16_t h = 20;
    const uint16_t bg = supportBadgeBg();
    const uint16_t fg = supportBadgeColor();

    gfx.fillRoundRect(x, y, w, h, 6, bg);
    gfx.drawRoundRect(x, y, w, h, 6, fg);
    gfx.setTextColor(fg, bg);
    gfx.drawCentreString(supportBadgeText(), x + w / 2, y + 5, 1);
}

template <typename Canvas>
void drawInfoCard(Canvas& gfx)
{
    const int16_t x = 8;
    const int16_t y = 58;
    const int16_t w = gfx.width() - 16;
    const int16_t h = 80;
    const int16_t labelX = x + 10;
    const int16_t valueX = x + 50;
    const int16_t row1Y = y + 34;
    const int16_t row2Y = y + 48;
    const int16_t row3Y = y + 62;

    gfx.fillRoundRect(x, y, w, h, 6, kUiCard);
    gfx.drawRoundRect(x, y, w, h, 6, kUiEdge);

    gfx.setTextColor(TFT_WHITE, kUiCard);
    gfx.drawString("HW: " + hardwareVersionText(), x + 10, y + 8, 2);
    gfx.drawFastHLine(x + 10, y + 24, w - 20, kUiEdge);

    gfx.setTextColor(kUiMuted, kUiCard);
    gfx.drawString("SW:", labelX, row1Y, 1);
    gfx.drawString("Build:", labelX, row2Y, 1);
    gfx.drawString("Git:", labelX, row3Y, 1);

    gfx.setTextColor(TFT_WHITE, kUiCard);
    gfx.drawString(kFactorySoftwareVersion, valueX, row1Y, 1);
    gfx.drawString(kFactoryBuildStamp, valueX, row2Y, 1);
    gfx.drawString(kFactoryGitHash, valueX, row3Y, 1);
}

template <typename Canvas>
void drawProbeLines(Canvas& gfx)
{
    char probeLine[64];
    snprintf(probeLine, sizeof(probeLine), "I2C: 22[%c] 24[%c] 55[%c] 6A[%c] 6B[%c]",
             hasAddr22 ? 'Y' : '-',
             hasAddr24 ? 'Y' : '-',
             hasAddr55 ? 'Y' : '-',
             hasAddr6A ? 'Y' : '-',
             hasAddr6B ? 'Y' : '-');

    gfx.setTextColor(kUiMuted, kUiBg);
    gfx.drawString(probeLine, 8, 142, 1);

    gfx.setTextColor(allowMenuEntry ? TFT_GREEN : TFT_RED, kUiBg);
    gfx.drawString(clipText(supportText(), 31), 106, 38, 1);
}

template <typename Canvas>
void drawUi(Canvas& gfx)
{
    gfx.fillScreen(kUiBg);
    drawHeader(gfx);
    drawStatusBadge(gfx);
    drawInfoCard(gfx);
    drawProbeLines(gfx);
    drawFooter(gfx);
}
}  // namespace

void init()
{
    canvasReady = false;
    screenDirty = true;
    enterMenuRequested = false;
    allowMenuEntry = true;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;

    canvas.setColorDepth(16);
    canvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!canvasReady) {
        Serial.println(F("[BOOT] Startup sprite allocation failed, using direct TFT redraw."));
    }

    detectHardwareProfile();
}

void update()
{
    bool activity = false;
    if (takeUserButton()) {
        activity = true;
    }
    if (takeEncoderButton()) {
        activity = true;
    }
    if (takeEncoderDelta(encSnapshot) != 0) {
        activity = true;
    }

    if (activity && allowMenuEntry) {
        enterMenuRequested = true;
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    if (canvasReady) {
        drawUi(canvas);
        canvas.pushSprite(0, 0);
    } else {
        drawUi(tft);
    }
    board_spi_deselect_all();

    screenDirty = false;
    lastDrawMs = now;
}

bool shouldEnterMainMenu()
{
    return enterMenuRequested;
}

void deinit()
{
    canvas.deleteSprite();
    canvasReady = false;
}

}  // namespace page_startup
