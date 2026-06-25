#pragma once

namespace page_wifi {

namespace {
enum class WiFiState : uint8_t {
    Idle = 0,
    Scanning,
    Connecting,
    Connected,
    NoTarget,
    Failed,
};

constexpr uint32_t kFrameMs = 80;
constexpr uint32_t kConnTimeoutMs = 15000;
constexpr uint32_t kStatusRefreshMs = 250;
constexpr uint32_t kConnectedRefreshMs = 1000;

constexpr char kSsid1[] = "LilyGo-AABB";
constexpr char kPassword1[] = "xinyuandianzi";
constexpr char kSsid2[] = "xinyuandianzi";
constexpr char kPassword2[] = "AA15994823428";

WiFiState state = WiFiState::Idle;
bool backFocused = false;
bool screenDirty = true;
uint32_t scanStartedMs = 0;
uint32_t connectStartedMs = 0;
uint32_t lastStatusTickMs = 0;
uint32_t lastConnectedRefreshMs = 0;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;
int scanCount = 0;
String ap0 = "-";
String ap1 = "-";
String ap2 = "-";
int32_t rssi0 = INT32_MIN;
String targetSsid = "-";
String connectedSsid = "-";
String connectedIp = "-";
int32_t targetRssi = INT32_MIN;
int32_t connectedRssi = INT32_MIN;
uint8_t connectAttempt = 0;
uint8_t connectTargetCount = 0;
const char* connectSsids[2] = {nullptr, nullptr};
const char* connectPasswords[2] = {nullptr, nullptr};
int32_t connectRssis[2] = {INT32_MIN, INT32_MIN};

String clipLine(String text, const uint8_t maxChars)
{
    if (text.length() <= maxChars) {
        return text;
    }
    return text.substring(0, maxChars - 3) + "...";
}

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

int32_t rssiForKnown(const char* ssid)
{
    for (int index = 0; index < scanCount; ++index) {
        if (WiFi.SSID(index) == ssid) {
            return WiFi.RSSI(index);
        }
    }
    return INT32_MIN;
}

void resetConnectionState()
{
    targetSsid = "-";
    connectedSsid = "-";
    connectedIp = "-";
    targetRssi = INT32_MIN;
    connectedRssi = INT32_MIN;
    connectAttempt = 0;
    connectTargetCount = 0;
    lastConnectedRefreshMs = 0;
    connectSsids[0] = nullptr;
    connectSsids[1] = nullptr;
    connectPasswords[0] = nullptr;
    connectPasswords[1] = nullptr;
    connectRssis[0] = INT32_MIN;
    connectRssis[1] = INT32_MIN;
}

void beginConnectAttempt(const uint8_t attemptIndex)
{
    if (attemptIndex >= connectTargetCount || connectSsids[attemptIndex] == nullptr) {
        WiFi.disconnect(true);
        state = WiFiState::Failed;
        screenDirty = true;
        Serial.println(F("[WiFi] Known AP connection failed."));
        return;
    }

    connectAttempt = attemptIndex;
    targetSsid = connectSsids[attemptIndex];
    targetRssi = connectRssis[attemptIndex];
    connectStartedMs = millis();
    lastStatusTickMs = 0;
    state = WiFiState::Connecting;
    screenDirty = true;

    WiFi.disconnect(true);
    delay(100);
    Serial.printf("[WiFi] Trying #%u: %s (%ld dBm)\n",
                  static_cast<unsigned>(attemptIndex + 1U),
                  targetSsid.c_str(),
                  static_cast<long>(targetRssi));
    WiFi.begin(connectSsids[attemptIndex], connectPasswords[attemptIndex]);
}

void prepareKnownConnections()
{
    connectTargetCount = 0;

    const int32_t rssi1 = rssiForKnown(kSsid1);
    const int32_t rssi2 = rssiForKnown(kSsid2);
    const bool found1 = rssi1 > INT32_MIN;
    const bool found2 = rssi2 > INT32_MIN;

    Serial.printf("[WiFi] Known APs: %s=%ld dBm, %s=%ld dBm\n",
                  kSsid1,
                  static_cast<long>(rssi1),
                  kSsid2,
                  static_cast<long>(rssi2));

    if (!found1 && !found2) {
        state = WiFiState::NoTarget;
        screenDirty = true;
        WiFi.scanDelete();
        Serial.println(F("[WiFi] No known AP found, skipping connection."));
        return;
    }

    if (found1 && found2) {
        if (rssi2 > rssi1) {
            connectSsids[0] = kSsid2;
            connectPasswords[0] = kPassword2;
            connectRssis[0] = rssi2;
            connectSsids[1] = kSsid1;
            connectPasswords[1] = kPassword1;
            connectRssis[1] = rssi1;
        } else {
            connectSsids[0] = kSsid1;
            connectPasswords[0] = kPassword1;
            connectRssis[0] = rssi1;
            connectSsids[1] = kSsid2;
            connectPasswords[1] = kPassword2;
            connectRssis[1] = rssi2;
        }
        connectTargetCount = 2;
    } else if (found1) {
        connectSsids[0] = kSsid1;
        connectPasswords[0] = kPassword1;
        connectRssis[0] = rssi1;
        connectTargetCount = 1;
    } else {
        connectSsids[0] = kSsid2;
        connectPasswords[0] = kPassword2;
        connectRssis[0] = rssi2;
        connectTargetCount = 1;
    }

    WiFi.scanDelete();
    beginConnectAttempt(0);
}

void startScan()
{
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    delay(80);

    scanCount = 0;
    ap0 = ap1 = ap2 = "-";
    rssi0 = INT32_MIN;
    resetConnectionState();
    scanStartedMs = millis();
    lastStatusTickMs = 0;
    state = WiFiState::Scanning;
    screenDirty = true;
    WiFi.scanNetworks(true);
}

template <typename Canvas>
void drawBackButton(Canvas& gfx, const bool selected)
{
    const int16_t w = 58;
    const int16_t h = 14;
    const int16_t x = gfx.width() - w - 6;
    const int16_t y = gfx.height() - kFooterH + 2;
    const uint16_t bg = selected ? TFT_WHITE : TFT_DARKGREY;
    const uint16_t fg = selected ? TFT_BLACK : TFT_LIGHTGREY;

    gfx.fillRoundRect(x, y, w, h, 5, bg);
    gfx.drawRoundRect(x, y, w, h, 5, selected ? TFT_YELLOW : 0x52AA);
    gfx.setTextColor(fg, bg);
    gfx.drawCentreString("BACK", x + w / 2, y + 3, 1);
}

template <typename Canvas>
void drawHeader(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderH, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("WiFi", kMargin, 5, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("Factory", gfx.width() - kMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas& gfx)
{
    const int16_t y = gfx.height() - kFooterH;
    gfx.fillRect(0, y, gfx.width(), kFooterH, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(backFocused ? "BOOT=back" : "BOOT=rescan", kMargin, y + 4, 1);
    drawBackButton(gfx, backFocused);
}

template <typename Canvas>
void drawStatusPill(Canvas& gfx, const char* label, const bool ok, const int16_t x, const int16_t y)
{
    const uint16_t bg = ok ? kPassBg : kFailBg;
    const uint16_t fg = ok ? TFT_GREEN : TFT_RED;
    gfx.fillRoundRect(x, y, 72, 20, 6, bg);
    gfx.drawRoundRect(x, y, 72, 20, 6, ok ? TFT_DARKGREEN : TFT_MAROON);
    gfx.setTextColor(fg, bg);
    gfx.drawCentreString(label, x + 36, y + 5, 1);
}

template <typename Canvas>
void drawMeter(Canvas& gfx, const int16_t x, const int16_t y, const int16_t w, const int16_t h,
               const uint16_t value, const uint16_t maxValue, const uint16_t color)
{
    const uint16_t clipped = min<uint16_t>(value, maxValue);
    const int16_t fillW = maxValue == 0 ? 0 : static_cast<int16_t>((static_cast<uint32_t>(w - 4) * clipped) / maxValue);
    gfx.fillRoundRect(x, y, w, h, 5, 0x2104);
    gfx.drawRoundRect(x, y, w, h, 5, kUiEdge);
    gfx.fillRoundRect(x + 2, y + 2, fillW, h - 4, 4, color);
}

template <typename Canvas>
void drawUi(Canvas& gfx)
{
    gfx.fillScreen(kUiBg);
    drawHeader(gfx);
    drawFooter(gfx);

    const bool ok = state == WiFiState::Connected;
    const char* pillText = "SCAN";
    if (state == WiFiState::Connecting) {
        pillText = "TRY";
    } else if (state == WiFiState::Connected) {
        pillText = "JOINED";
    } else if (state == WiFiState::NoTarget) {
        pillText = "IDLE";
    } else if (state == WiFiState::Failed) {
        pillText = "FAIL";
    }
    drawStatusPill(gfx, pillText, ok, 8, 34);

    gfx.setTextColor(TFT_WHITE, kUiBg);
    if (state == WiFiState::Scanning) {
        gfx.drawString(String("Scanning... ") + ((millis() - scanStartedMs) / 1000U) + "s", 88, 38, 2);
    } else if (state == WiFiState::Connecting) {
        gfx.drawString("Trying: " + clipLine(targetSsid, 18), 88, 38, 2);
    } else if (state == WiFiState::Connected) {
        gfx.drawString("Connected: " + clipLine(connectedSsid, 14), 88, 38, 2);
    } else if (state == WiFiState::NoTarget) {
        gfx.drawString("Known AP not found", 88, 38, 2);
    } else if (state == WiFiState::Failed) {
        gfx.drawString("Known AP connect fail", 88, 38, 2);
    } else {
        gfx.drawString(String("Networks: ") + scanCount, 88, 38, 2);
    }

    gfx.setTextColor(TFT_GREEN, kUiBg);
    gfx.drawString("1 " + ap0, 8, 74, 1);
    gfx.setTextColor(TFT_CYAN, kUiBg);
    gfx.drawString("2 " + ap1, 8, 96, 1);
    gfx.setTextColor(TFT_YELLOW, kUiBg);
    gfx.drawString("3 " + ap2, 8, 118, 1);

    int32_t meterRssi = rssi0;
    gfx.fillRect(0, 128, gfx.width(), 24, kUiBg);

    if (state == WiFiState::Connecting && targetRssi > INT32_MIN) {
        gfx.setTextColor(TFT_WHITE, kUiBg);
        gfx.drawString(String("Target: ") + clipLine(targetSsid, 16), 8, 132, 1);
        meterRssi = targetRssi;
    } else if (state == WiFiState::Connected && connectedRssi > INT32_MIN) {
        gfx.setTextColor(TFT_WHITE, kUiBg);
        gfx.drawString(String("RSSI: ") + connectedRssi + " dBm", 8, 132, 1);
        gfx.drawString("IP: " + clipLine(connectedIp, 23), 8, 142, 1);
        meterRssi = connectedRssi;
    } else if (state == WiFiState::NoTarget) {
        gfx.setTextColor(TFT_WHITE, kUiBg);
        gfx.drawString("Only scan, no auto connect", 8, 136, 1);
    } else if (state == WiFiState::Failed) {
        gfx.setTextColor(TFT_WHITE, kUiBg);
        gfx.drawString("Retry with BOOT", 8, 136, 1);
    } else if (rssi0 > INT32_MIN) {
        gfx.setTextColor(TFT_WHITE, kUiBg);
        gfx.drawString(String("Top RSSI: ") + rssi0 + " dBm", 8, 136, 1);
    }

    if (meterRssi > INT32_MIN) {
        drawMeter(gfx, 206, 138, 106, 10, static_cast<uint16_t>(max<int32_t>(0, meterRssi + 100)), 70,
                  state == WiFiState::Connected ? TFT_GREEN : TFT_CYAN);
    }
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

    if (state == WiFiState::Connecting) {
        const wl_status_t wifiStatus = WiFi.status();
        if (wifiStatus == WL_CONNECTED) {
            connectedSsid = WiFi.SSID();
            connectedIp = WiFi.localIP().toString();
            connectedRssi = WiFi.RSSI();
            state = WiFiState::Connected;
            screenDirty = true;
            Serial.printf("[WiFi] Connected: %s  IP=%s  RSSI=%ld\n",
                          connectedSsid.c_str(),
                          connectedIp.c_str(),
                          static_cast<long>(connectedRssi));
            return;
        }

        const uint32_t now = millis();
        if ((now - connectStartedMs) > kConnTimeoutMs) {
            Serial.printf("[WiFi] Timeout: %s\n", targetSsid.c_str());
            beginConnectAttempt(static_cast<uint8_t>(connectAttempt + 1U));
            return;
        }

        if (lastStatusTickMs == 0 || (now - lastStatusTickMs) >= kStatusRefreshMs) {
            lastStatusTickMs = now;
            screenDirty = true;
        }
        return;
    }

    if (state == WiFiState::Connected) {
        const uint32_t now = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(F("[WiFi] Connection lost, restarting scan."));
            startScan();
            return;
        }

        if (lastConnectedRefreshMs == 0 || (now - lastConnectedRefreshMs) >= kConnectedRefreshMs) {
            lastConnectedRefreshMs = now;
            const int32_t newRssi = WiFi.RSSI();
            const String newIp = WiFi.localIP().toString();
            if (newRssi != connectedRssi || newIp != connectedIp) {
                connectedRssi = newRssi;
                connectedIp = newIp;
                screenDirty = true;
            }
        }
        return;
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
    prepareKnownConnections();
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    drawUi(tft);
    board_spi_deselect_all();
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
