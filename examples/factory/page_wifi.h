#pragma once

namespace page_wifi {

namespace {
enum class WiFiState : uint8_t {
    Idle = 0,
    Scanning,
    Done,
    Failed,
};

constexpr uint32_t kFrameMs = 80;

WiFiState state = WiFiState::Idle;
bool backFocused = false;
bool screenDirty = true;
uint32_t scanStartedMs = 0;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;
int scanCount = 0;
String ap0 = "-";
String ap1 = "-";
String ap2 = "-";
int32_t rssi0 = INT32_MIN;

String apLine(const int index)
{
    if (index < 0 || index >= scanCount) {
        return "-";
    }
    String line = WiFi.SSID(index) + "  " + String(WiFi.RSSI(index)) + " dBm";
    if (line.length() > 42) {
        line = line.substring(0, 39) + "...";
    }
    return line;
}

void startScan()
{
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    delay(80);

    scanCount = 0;
    ap0 = ap1 = ap2 = "-";
    rssi0 = INT32_MIN;
    scanStartedMs = millis();
    state = WiFiState::Scanning;
    screenDirty = true;
    WiFi.scanNetworks(true);
}
}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;
    startScan();
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
        startScan();
    }

    if (state != WiFiState::Scanning) {
        return;
    }

    const int result = WiFi.scanComplete();
    if (result == WIFI_SCAN_RUNNING) {
        if (((millis() - scanStartedMs) % 500U) < 20U) {
            screenDirty = true;
        }
        return;
    }

    if (result < 0) {
        state = WiFiState::Failed;
        screenDirty = true;
        return;
    }

    scanCount = result;
    if (scanCount > 0) {
        ap0 = apLine(0);
        rssi0 = WiFi.RSSI(0);
    }
    if (scanCount > 1) {
        ap1 = apLine(1);
    }
    if (scanCount > 2) {
        ap2 = apLine(2);
    }
    WiFi.scanDelete();
    state = WiFiState::Done;
    screenDirty = true;
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    gSubPageGfx.beginFrame();
    tft.fillScreen(kUiBg);
    drawPageHeader("WiFi", TFT_CYAN);
    drawPageFooter("BOOT=rescan", backFocused);

    const bool ok = state == WiFiState::Done && scanCount > 0;
    drawStatusPill(ok ? "AP FOUND" : (state == WiFiState::Failed ? "FAIL" : "SCAN"), ok, 8, 34);
    tft.setTextColor(TFT_WHITE, kUiBg);
    if (state == WiFiState::Scanning) {
        tft.drawString(String("Scanning... ") + ((millis() - scanStartedMs) / 1000U) + "s", 88, 38, 2);
    } else if (state == WiFiState::Failed) {
        tft.drawString("Scan failed", 88, 38, 2);
    } else {
        tft.drawString(String("Networks: ") + scanCount, 88, 38, 2);
    }

    tft.setTextColor(TFT_GREEN, kUiBg);
    tft.drawString("1 " + ap0, 8, 74, 1);
    tft.setTextColor(TFT_CYAN, kUiBg);
    tft.drawString("2 " + ap1, 8, 96, 1);
    tft.setTextColor(TFT_YELLOW, kUiBg);
    tft.drawString("3 " + ap2, 8, 118, 1);
    if (rssi0 > INT32_MIN) {
        tft.setTextColor(TFT_WHITE, kUiBg);
        tft.drawString(String("Top RSSI: ") + rssi0 + " dBm", 8, 138, 1);
    }
    if (rssi0 > INT32_MIN) {
        drawMeter(168, 138, 144, 10, static_cast<uint16_t>(max<int32_t>(0, rssi0 + 100)), 70, TFT_GREEN);
    }

    board_spi_deselect_all();
    gSubPageGfx.endFrame();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

}  // namespace page_wifi
