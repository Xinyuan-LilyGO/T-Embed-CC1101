#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "utilities.h"

namespace {

constexpr char kWifiApSsid[] = "T-Embed-RF-Test";
constexpr char kWifiApPassword[] = "12345678";
constexpr uint8_t kWifiApChannel = 6;
constexpr uint8_t kWifiApMaxClients = 4;

constexpr uint32_t kFirstWifiScanDelayMs = 1000;
constexpr uint32_t kWifiScanIntervalMs = 10000;
constexpr uint32_t kDisplayRefreshMs = 1000;
constexpr uint32_t kSerialReportMs = 5000;
constexpr int kMaxNetworksShown = 5;

struct NetworkSummary {
    String ssid;
    int32_t rssi = INT32_MIN;
    int32_t channel = 0;
    wifi_auth_mode_t auth = WIFI_AUTH_OPEN;
};

TFT_eSPI tft;
NetworkSummary topNetworks[kMaxNetworksShown];

int scanCount = -1;
int8_t wifiTxPowerQuarterDbm = 0;
uint32_t lastWifiScanMs = 0;
uint32_t lastDisplayMs = 0;
uint32_t lastSerialReportMs = 0;

String clipText(const String &value, uint8_t maxLen)
{
    if (value.length() <= maxLen) {
        return value;
    }
    if (maxLen <= 3) {
        return value.substring(0, maxLen);
    }
    return value.substring(0, maxLen - 3) + "...";
}

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

    pinMode(BOARD_NRF24_CE, OUTPUT);
    digitalWrite(BOARD_NRF24_CE, LOW);
}

void initDisplay()
{
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);

    deselectSharedSpiDevices();

    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    tft.begin();
    tft.setRotation(3);
    tft.setTextWrap(false);
    tft.fillScreen(TFT_BLACK);
    digitalWrite(DISPLAY_BL, HIGH);
}

void clearNetworkList()
{
    for (auto &network : topNetworks) {
        network.ssid = "-";
        network.rssi = INT32_MIN;
        network.channel = 0;
        network.auth = WIFI_AUTH_OPEN;
    }
}

void insertNetwork(const String &ssid, int32_t rssi, int32_t channel, wifi_auth_mode_t auth)
{
    for (int slot = 0; slot < kMaxNetworksShown; ++slot) {
        if (rssi <= topNetworks[slot].rssi) {
            continue;
        }

        for (int move = kMaxNetworksShown - 1; move > slot; --move) {
            topNetworks[move] = topNetworks[move - 1];
        }

        topNetworks[slot].ssid = ssid.length() ? ssid : "<hidden>";
        topNetworks[slot].rssi = rssi;
        topNetworks[slot].channel = channel;
        topNetworks[slot].auth = auth;
        break;
    }
}

void updateTopNetworks()
{
    clearNetworkList();

    if (scanCount <= 0) {
        return;
    }

    for (int index = 0; index < scanCount; ++index) {
        insertNetwork(WiFi.SSID(index),
                      WiFi.RSSI(index),
                      WiFi.channel(index),
                      WiFi.encryptionType(index));
    }
}

uint16_t rssiColor(int32_t rssi)
{
    if (rssi >= -55) {
        return TFT_GREEN;
    }
    if (rssi >= -67) {
        return TFT_CYAN;
    }
    if (rssi >= -75) {
        return TFT_YELLOW;
    }
    return TFT_RED;
}

void drawRssiBar(int16_t x, int16_t y, int16_t width, int16_t height, int32_t rssi)
{
    tft.drawRect(x, y, width, height, TFT_DARKGREY);

    if (rssi <= INT32_MIN) {
        return;
    }

    const int32_t clamped = constrain(rssi, -95, -35);
    const int16_t fillWidth = map(clamped, -95, -35, 0, width - 2);
    tft.fillRect(x + 1, y + 1, fillWidth, height - 2, rssiColor(rssi));
}

void drawDisplay()
{
    tft.fillScreen(TFT_BLACK);

    tft.fillRect(0, 0, tft.width(), 24, TFT_CYAN);
    tft.setTextColor(TFT_BLACK, TFT_CYAN);
    tft.drawString("WiFi RF Signal Test", 8, 5, 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String("WiFi AP: ") + kWifiApSsid, 8, 32, 2);
    tft.drawString(String("Channel: ") + kWifiApChannel, 8, 50, 2);

    const float wifiTxPowerDbm = wifiTxPowerQuarterDbm / 4.0f;
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(String("WiFi TX max: ") + String(wifiTxPowerDbm, 1) + " dBm", 8, 70, 1);
    tft.drawString(String("AP clients: ") + WiFi.softAPgetStationNum(), 164, 70, 1);
    tft.drawString(String("Scan count: ") + scanCount, 8, 84, 1);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Strongest WiFi routers nearby", 8, 100, 1);

    for (int row = 0; row < kMaxNetworksShown; ++row) {
        const int16_t y = 116 + row * 10;
        const NetworkSummary &network = topNetworks[row];

        if (network.rssi <= INT32_MIN) {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString(String(row + 1) + ". -", 8, y, 1);
            continue;
        }

        const String authText = (network.auth == WIFI_AUTH_OPEN) ? "open" : "sec";
        const String line = String(row + 1) + ". " + clipText(network.ssid, 18) +
                            " ch" + network.channel + " " + network.rssi + "dBm " + authText;

        tft.setTextColor(rssiColor(network.rssi), TFT_BLACK);
        tft.drawString(line, 8, y, 1);
        drawRssiBar(250, y + 1, 56, 7, network.rssi);
    }
}

void printHeader()
{
    Serial.println();
    Serial.println(F("==================================================="));
    Serial.println(F("T-Embed CC1101 Plus WiFi signal diagnostic"));
    Serial.println(F("==================================================="));
    Serial.println(F("What this sketch does:"));
    Serial.println(F("1. Starts a 2.4 GHz WiFi AP named T-Embed-RF-Test."));
    Serial.println(F("2. Sets WiFi max TX power and disables WiFi sleep."));
    Serial.println(F("3. Scans nearby routers and prints RSSI in dBm."));
    Serial.println();
    Serial.println(F("Customer test steps:"));
    Serial.println(F("1. Keep the external antenna installed before power on."));
    Serial.println(F("2. Open phone WiFi list/analyzer and check T-Embed-RF-Test RSSI."));
    Serial.println(F("3. Record RSSI at 0.5 m, 1.5 m, and 3 m."));
    Serial.println(F("4. Put the device next to the router and compare RSSI readings."));
    Serial.println(F("5. If WiFi is still weak at close range, check antenna/RF path hardware."));
    Serial.println();
}

void printScanReport()
{
    Serial.println();
    Serial.printf("[WiFi] AP SSID: %s  password: %s  channel: %u  clients: %u\n",
                  kWifiApSsid,
                  kWifiApPassword,
                  static_cast<unsigned>(kWifiApChannel),
                  static_cast<unsigned>(WiFi.softAPgetStationNum()));
    Serial.printf("[WiFi] Max TX power: %.1f dBm (%d quarter-dBm units)\n",
                  wifiTxPowerQuarterDbm / 4.0f,
                  static_cast<int>(wifiTxPowerQuarterDbm));

    if (scanCount < 0) {
        Serial.println(F("[WiFi] No successful router scan yet."));
        return;
    }

    Serial.printf("[WiFi] Scan result: %d networks\n", scanCount);
    for (int row = 0; row < kMaxNetworksShown; ++row) {
        const NetworkSummary &network = topNetworks[row];
        if (network.rssi <= INT32_MIN) {
            continue;
        }

        Serial.printf("  #%d RSSI=%ld dBm  CH=%ld  %s  SSID=%s\n",
                      row + 1,
                      static_cast<long>(network.rssi),
                      static_cast<long>(network.channel),
                      (network.auth == WIFI_AUTH_OPEN) ? "OPEN" : "SEC",
                      network.ssid.c_str());
    }
}

void configureWifiPower()
{
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);

    esp_err_t result = esp_wifi_set_max_tx_power(78);
    if (result != ESP_OK) {
        Serial.printf("[WiFi] esp_wifi_set_max_tx_power failed: %s\n", esp_err_to_name(result));
    }

    result = esp_wifi_get_max_tx_power(&wifiTxPowerQuarterDbm);
    if (result != ESP_OK) {
        Serial.printf("[WiFi] esp_wifi_get_max_tx_power failed: %s\n", esp_err_to_name(result));
        wifiTxPowerQuarterDbm = 0;
    }
}

void startWifiAp()
{
    const bool apStarted = WiFi.softAP(kWifiApSsid,
                                       kWifiApPassword,
                                       kWifiApChannel,
                                       false,
                                       kWifiApMaxClients);
    Serial.printf("[WiFi] softAP %s, IP=%s\n",
                  apStarted ? "started" : "failed",
                  WiFi.softAPIP().toString().c_str());
}

void startWifiScan()
{
    WiFi.scanDelete();
    Serial.println(F("[WiFi] Starting router scan..."));

    // A blocking scan is more reliable than the async scan path on this board
    // while SoftAP is active, and avoids repeated WIFI_SCAN_FAILED (-2) logs.
    scanCount = WiFi.scanNetworks(false, true);
    lastWifiScanMs = millis();

    if (scanCount < 0) {
        Serial.printf("[WiFi] Scan failed: %d\n", scanCount);
        WiFi.scanDelete();
        return;
    }

    updateTopNetworks();
    printScanReport();
    WiFi.scanDelete();
}

void serviceWifiScan()
{
    if ((millis() - lastWifiScanMs) < kWifiScanIntervalMs) {
        return;
    }
    startWifiScan();
}

void serviceDisplay()
{
    if ((millis() - lastDisplayMs) < kDisplayRefreshMs) {
        return;
    }

    lastDisplayMs = millis();
    drawDisplay();
}

void serviceSerialReport()
{
    if ((millis() - lastSerialReportMs) < kSerialReportMs) {
        return;
    }

    lastSerialReportMs = millis();
    printScanReport();
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    printHeader();
    clearNetworkList();
    initDisplay();

    // BLE is intentionally disabled here so customer testing focuses only on
    // WiFi performance and avoids RF coexistence-related restart issues.
    Serial.println(F("[APP] WiFi-only test mode"));
    configureWifiPower();
    startWifiAp();
    drawDisplay();

    delay(kFirstWifiScanDelayMs);
    startWifiScan();
}

void loop()
{
    serviceWifiScan();
    serviceDisplay();
    serviceSerialReport();
    delay(20);
}
