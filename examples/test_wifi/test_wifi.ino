#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

#include "utilities.h"

// ---- Configure your two candidate networks here ----
static const char *kSsid1 = "YourSSID_1";
static const char *kPassword1 = "YourPassword1";
static const char *kSsid2 = "YourSSID_2";
static const char *kPassword2 = "YourPassword2";
// ----------------------------------------------------

static constexpr uint8_t kRotation = 3;
static constexpr uint32_t kConnTimeoutMs = 15000;
static constexpr uint32_t kRetryDelayMs = 10000;

static TFT_eSPI tft;

static int16_t lineY(uint8_t row)
{
    return 36 + row * 18;
}

static void deselectSharedSpiDevices()
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

static void drawHeader(const char *title, uint16_t color)
{
    tft.fillRect(0, 0, tft.width(), 28, color);
    tft.setTextColor(TFT_BLACK, color);
    tft.drawString(title, 8, 7, 2);
}

static void clearBody()
{
    tft.fillRect(0, 28, tft.width(), tft.height() - 28, TFT_BLACK);
}

static bool initBoardForDisplay()
{
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);

    deselectSharedSpiDevices();

    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    tft.begin();
    tft.setRotation(kRotation);
    tft.fillScreen(TFT_BLACK);

    digitalWrite(DISPLAY_BL, HIGH);
    return true;
}

static int32_t rssiFor(const char *ssid)
{
    int count = WiFi.scanComplete();
    for (int index = 0; index < count; ++index) {
        if (WiFi.SSID(index) == ssid) {
            return WiFi.RSSI(index);
        }
    }
    return INT32_MIN;
}

static void drawRssiBars(int16_t x, int16_t y, int32_t rssi, uint16_t color)
{
    int bars = 0;
    if (rssi > INT32_MIN) {
        if (rssi >= -50) {
            bars = 5;
        } else if (rssi >= -60) {
            bars = 4;
        } else if (rssi >= -70) {
            bars = 3;
        } else if (rssi >= -80) {
            bars = 2;
        } else {
            bars = 1;
        }
    }

    for (int index = 0; index < 5; ++index) {
        const int16_t barX = x + index * 7;
        const int16_t barH = 4 + index * 3;
        const int16_t barY = y + 15 - barH;
        const uint16_t barColor = (index < bars) ? color : TFT_DARKGREY;
        tft.fillRect(barX, barY, 5, barH, barColor);
    }
}

static void drawScanPage(const char *message)
{
    tft.fillScreen(TFT_BLACK);
    drawHeader("WiFi Test", TFT_CYAN);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Scanning...", 8, lineY(0), 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(message, 8, lineY(1), 1);
}

static void drawSuccess(const char *ssid, IPAddress ip, int32_t rssi)
{
    tft.fillScreen(TFT_BLACK);
    drawHeader("WiFi  Connected", TFT_GREEN);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Connected!", 8, lineY(0), 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("SSID:", 8, lineY(1), 2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(String(ssid), 8, lineY(2), 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("IP:", 8, lineY(3), 2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(ip.toString(), 8, lineY(4), 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String("Signal: ") + String(rssi) + " dBm", 8, lineY(5), 2);
    drawRssiBars(8, lineY(6), rssi, TFT_GREEN);
}

static void drawTrying(const char *ssid, int32_t rssi, uint8_t attempt, uint32_t elapsedMs)
{
    clearBody();

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(String("Trying #") + attempt + "...", 8, lineY(0), 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String("SSID: ") + ssid, 8, lineY(1), 2);

    const String rssiText = (rssi > INT32_MIN) ? (String(rssi) + " dBm") : "N/A";
    tft.drawString(String("RSSI: ") + rssiText, 8, lineY(2), 2);
    drawRssiBars(8, lineY(3), rssi, TFT_GREEN);

    const uint8_t dots = (elapsedMs / 500) % 4;
    String dotText;
    for (uint8_t index = 0; index < dots; ++index) {
        dotText += '.';
    }

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(dotText + "       ", 8, lineY(4) + 6, 4);
}

static void drawFailed(const char *ssid1, const char *ssid2)
{
    tft.fillScreen(TFT_BLACK);
    drawHeader("WiFi  Failed", TFT_RED);

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Both networks failed!", 8, lineY(0), 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(ssid1), 8, lineY(1), 2);
    tft.drawString(String(ssid2), 8, lineY(2), 2);

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Retrying in 10s...", 8, lineY(3), 1);
}

static bool tryConnect(const char *ssid, const char *password, int32_t rssi, uint8_t attempt)
{
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(ssid, password);

    clearBody();
    drawHeader("WiFi Test", TFT_CYAN);

    const uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED) {
        const uint32_t elapsedMs = millis() - startMs;
        if (elapsedMs > kConnTimeoutMs) {
            Serial.printf("[WiFi] Timeout: %s\n", ssid);
            return false;
        }

        drawTrying(ssid, rssi, attempt, elapsedMs);
        delay(200);
    }

    return true;
}

static bool runConnect()
{
    drawScanPage("Starting scan...");

    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.disconnect(true);
    delay(100);

    WiFi.scanNetworks(true);

    const uint32_t scanStartMs = millis();
    while (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
        const String message = String("Scanning... ") + ((millis() - scanStartMs) / 1000) + "s";
        drawScanPage(message.c_str());
        delay(300);
    }

    const int scanResult = WiFi.scanComplete();
    Serial.printf("[WiFi] Scan done: %d networks\n", scanResult);

    const int32_t rssi1 = rssiFor(kSsid1);
    const int32_t rssi2 = rssiFor(kSsid2);
    Serial.printf("[WiFi] %s RSSI=%ld  |  %s RSSI=%ld\n",
                  kSsid1,
                  static_cast<long>(rssi1),
                  kSsid2,
                  static_cast<long>(rssi2));

    const char *firstSsid = kSsid1;
    const char *firstPassword = kPassword1;
    int32_t firstRssi = rssi1;

    const char *secondSsid = kSsid2;
    const char *secondPassword = kPassword2;
    int32_t secondRssi = rssi2;

    if (rssi2 > rssi1) {
        firstSsid = kSsid2;
        firstPassword = kPassword2;
        firstRssi = rssi2;

        secondSsid = kSsid1;
        secondPassword = kPassword1;
        secondRssi = rssi1;
    }

    Serial.printf("[WiFi] Trying first: %s\n", firstSsid);
    if (tryConnect(firstSsid, firstPassword, firstRssi, 1)) {
        const int32_t connectedRssi = WiFi.RSSI();
        const IPAddress ip = WiFi.localIP();
        Serial.printf("[WiFi] Connected! IP=%s RSSI=%ld\n",
                      ip.toString().c_str(),
                      static_cast<long>(connectedRssi));
        drawSuccess(firstSsid, ip, connectedRssi);
        WiFi.scanDelete();
        return true;
    }

    Serial.printf("[WiFi] First failed, trying: %s\n", secondSsid);
    if (tryConnect(secondSsid, secondPassword, secondRssi, 2)) {
        const int32_t connectedRssi = WiFi.RSSI();
        const IPAddress ip = WiFi.localIP();
        Serial.printf("[WiFi] Connected! IP=%s RSSI=%ld\n",
                      ip.toString().c_str(),
                      static_cast<long>(connectedRssi));
        drawSuccess(secondSsid, ip, connectedRssi);
        WiFi.scanDelete();
        return true;
    }

    Serial.println(F("[WiFi] Both networks failed."));
    drawFailed(kSsid1, kSsid2);
    WiFi.scanDelete();
    return false;
}

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("T-Embed WiFi test"));

    if (!initBoardForDisplay()) {
        Serial.println(F("[WiFi] Display init failed. Halting."));
        while (true) {
            delay(1000);
        }
    }
}

void loop()
{
    const bool connected = runConnect();

    if (connected) {
        for (int index = 0; index < 200; ++index) {
            delay(3000);

            const int32_t rssi = WiFi.RSSI();
            Serial.printf("[WiFi] RSSI: %ld dBm\n", static_cast<long>(rssi));

            tft.fillRect(8, lineY(6), 50, 20, TFT_BLACK);
            drawRssiBars(8, lineY(6), rssi, TFT_GREEN);

            tft.fillRect(8, lineY(5), tft.width() - 8, 18, TFT_BLACK);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(String("Signal: ") + String(rssi) + " dBm", 8, lineY(5), 2);
        }

        WiFi.disconnect(true);
    } else {
        delay(kRetryDelayMs);
    }
}
