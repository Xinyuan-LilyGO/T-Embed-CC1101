#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <TFT_eSPI.h>
#include <esp_gap_ble_api.h>

#include <cstring>
#include <string>

#include "utilities.h"

namespace {

constexpr char kDeviceName[] = "T-Embed-BLE-UART";
constexpr uint32_t kPasskey = 123456;

constexpr char kServiceUuid[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char kRxUuid[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char kTxUuid[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

constexpr uint16_t kInvalidConnId = 0xFFFF;
constexpr uint32_t kClearHoldMs = 2000;
constexpr uint32_t kAdvertisingRestartDelayMs = 500;
constexpr uint32_t kUiRefreshMs = 250;
constexpr uint32_t kMaxEchoBytes = 120;

constexpr size_t kStatusTextSize = 24;
constexpr size_t kAddressTextSize = 18;
constexpr size_t kLastRxTextSize = 40;

struct AppState {
    bool connected;
    bool advertising;
    bool authInProgress;
    bool authFailed;
    bool bonded;
    uint16_t connId;
    uint32_t passkey;
    uint32_t rxCount;
    uint32_t txCount;
    uint8_t bondCount;
    char status[kStatusTextSize];
    char localAddress[kAddressTextSize];
    char peerAddress[kAddressTextSize];
    char lastRx[kLastRxTextSize];
};

portMUX_TYPE gStateMux = portMUX_INITIALIZER_UNLOCKED;

TFT_eSPI gTft;
BLEServer *gServer = nullptr;
BLECharacteristic *gTxCharacteristic = nullptr;

AppState gState = {};

bool gDisplayDirty = true;
bool gRestartAdvertisingPending = false;
uint32_t gRestartAdvertisingAt = 0;
uint32_t gLastUiDrawMs = 0;

bool gKnobPressed = false;
bool gKnobActionLatched = false;
uint32_t gKnobPressedAt = 0;

void copyText(char *dest, size_t destSize, const char *value)
{
    if (destSize == 0) {
        return;
    }
    snprintf(dest, destSize, "%s", value ? value : "");
}

void formatPeerText(const std::string &value, char *dest, size_t destSize)
{
    if (destSize == 0) {
        return;
    }

    size_t writeIndex = 0;
    for (char ch : value) {
        if (writeIndex >= (destSize - 1)) {
            break;
        }

        const unsigned char uch = static_cast<unsigned char>(ch);
        if (uch == '\r' || uch == '\n' || uch == '\t') {
            dest[writeIndex++] = ' ';
        } else if (uch >= 32 && uch < 127) {
            dest[writeIndex++] = ch;
        } else {
            dest[writeIndex++] = '?';
        }
    }

    if (writeIndex == 0) {
        dest[writeIndex++] = '-';
    }
    dest[writeIndex] = '\0';
}

uint8_t getBondCount()
{
    const int count = esp_ble_get_bond_device_num();
    if (count <= 0) {
        return 0;
    }
    return static_cast<uint8_t>(count > 255 ? 255 : count);
}

void updateStatus(const char *status)
{
    portENTER_CRITICAL(&gStateMux);
    copyText(gState.status, sizeof(gState.status), status);
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);
}

void updatePeerAddress(const char *address)
{
    portENTER_CRITICAL(&gStateMux);
    copyText(gState.peerAddress, sizeof(gState.peerAddress), address);
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);
}

void updateLastRx(const std::string &value)
{
    char formatted[kLastRxTextSize];
    formatPeerText(value, formatted, sizeof(formatted));

    portENTER_CRITICAL(&gStateMux);
    copyText(gState.lastRx, sizeof(gState.lastRx), formatted);
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);
}

void updateBondState(bool bonded)
{
    const uint8_t bondCount = getBondCount();

    portENTER_CRITICAL(&gStateMux);
    gState.bonded = bonded;
    gState.bondCount = bondCount;
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);
}

void scheduleAdvertisingRestart(uint32_t delayMs)
{
    const uint32_t restartAt = millis() + delayMs;

    portENTER_CRITICAL(&gStateMux);
    gRestartAdvertisingPending = true;
    gRestartAdvertisingAt = restartAt;
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);
}

AppState snapshotState(bool *dirty)
{
    AppState snapshot;

    portENTER_CRITICAL(&gStateMux);
    snapshot = gState;
    if (dirty != nullptr) {
        *dirty = gDisplayDirty;
        gDisplayDirty = false;
    }
    portEXIT_CRITICAL(&gStateMux);

    return snapshot;
}

void drawLine(int16_t y, const char *text, uint16_t color)
{
    gTft.setCursor(4, y);
    gTft.setTextColor(color, TFT_BLACK);
    gTft.print(text);
}

uint16_t statusColor(const char *status)
{
    if (strcmp(status, "Advertising") == 0) {
        return TFT_CYAN;
    }
    if (strcmp(status, "Pairing") == 0) {
        return TFT_YELLOW;
    }
    if (strcmp(status, "Bonded") == 0 || strcmp(status, "Connected") == 0) {
        return TFT_GREEN;
    }
    if (strcmp(status, "Disconnected") == 0) {
        return TFT_ORANGE;
    }
    if (strcmp(status, "Auth Failed") == 0) {
        return TFT_RED;
    }
    if (strcmp(status, "Clearing Bonds") == 0) {
        return TFT_MAGENTA;
    }
    return TFT_WHITE;
}

void renderScreen()
{
    AppState snapshot = snapshotState(nullptr);

    gTft.fillScreen(TFT_BLACK);
    gTft.setTextFont(4);
    gTft.setTextColor(TFT_CYAN, TFT_BLACK);
    gTft.setCursor(4, 4);
    gTft.print("BLE UART");

    gTft.setTextFont(2);

    char line[96];

    snprintf(line, sizeof(line), "Name: %s", kDeviceName);
    drawLine(34, line, TFT_WHITE);

    snprintf(line, sizeof(line), "Status: %s", snapshot.status);
    drawLine(50, line, statusColor(snapshot.status));

    snprintf(line, sizeof(line), "Passkey: %06lu", static_cast<unsigned long>(snapshot.passkey));
    drawLine(66, line, snapshot.authInProgress ? TFT_YELLOW : TFT_WHITE);

    snprintf(line, sizeof(line), "Local: %s", snapshot.localAddress);
    drawLine(82, line, TFT_WHITE);

    snprintf(line, sizeof(line), "Peer: %s", snapshot.peerAddress[0] ? snapshot.peerAddress : "-");
    drawLine(98, line, snapshot.connected ? TFT_GREEN : TFT_LIGHTGREY);

    snprintf(line,
             sizeof(line),
             "Bond:%u RX:%lu TX:%lu",
             snapshot.bondCount,
             static_cast<unsigned long>(snapshot.rxCount),
             static_cast<unsigned long>(snapshot.txCount));
    drawLine(114, line, TFT_WHITE);

    snprintf(line, sizeof(line), "Last RX: %s", snapshot.lastRx);
    drawLine(130, line, TFT_WHITE);

    drawLine(146, "Hold knob to clear bonds", TFT_DARKGREY);
}

void boardPrepareSpi()
{
    pinMode(DISPLAY_CS, OUTPUT);
    digitalWrite(DISPLAY_CS, HIGH);

    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);

    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);
}

void clearBondedDevices()
{
    const int bondCount = esp_ble_get_bond_device_num();
    if (bondCount <= 0) {
        updateBondState(false);
        return;
    }

    auto *bondedDevices = static_cast<esp_ble_bond_dev_t *>(
        malloc(sizeof(esp_ble_bond_dev_t) * static_cast<size_t>(bondCount)));
    if (bondedDevices == nullptr) {
        Serial.println("[BLE] Failed to allocate bonded device list.");
        return;
    }

    int listCount = bondCount;
    const esp_err_t listResult = esp_ble_get_bond_device_list(&listCount, bondedDevices);
    if (listResult != ESP_OK) {
        Serial.printf("[BLE] Failed to get bond list: %d\n", listResult);
        free(bondedDevices);
        return;
    }

    for (int index = 0; index < listCount; ++index) {
        char address[kAddressTextSize];
        copyText(address, sizeof(address), BLEAddress(bondedDevices[index].bd_addr).toString().c_str());
        const esp_err_t result = esp_ble_remove_bond_device(bondedDevices[index].bd_addr);
        Serial.printf("[BLE] Remove bond %s -> %d\n", address, result);
    }

    free(bondedDevices);
    delay(100);
    updateBondState(false);
}

void clearConnectionsAndBonds()
{
    updateStatus("Clearing Bonds");

    AppState snapshot = snapshotState(nullptr);
    if (snapshot.connected && gServer != nullptr && snapshot.connId != kInvalidConnId) {
        Serial.printf("[BLE] Disconnecting conn_id=%u before clearing bonds.\n", snapshot.connId);
        gServer->disconnect(snapshot.connId);
        delay(150);
    }

    clearBondedDevices();

    portENTER_CRITICAL(&gStateMux);
    gState.connected = false;
    gState.authInProgress = false;
    gState.authFailed = false;
    gState.bonded = false;
    gState.connId = kInvalidConnId;
    copyText(gState.peerAddress, sizeof(gState.peerAddress), "");
    copyText(gState.lastRx, sizeof(gState.lastRx), "-");
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);

    scheduleAdvertisingRestart(200);
}

void serviceKnobHold()
{
    const bool pressed = (digitalRead(ENCODER_KEY) == LOW);

    if (pressed && !gKnobPressed) {
        gKnobPressed = true;
        gKnobActionLatched = false;
        gKnobPressedAt = millis();
    } else if (!pressed) {
        gKnobPressed = false;
        gKnobActionLatched = false;
    }

    if (gKnobPressed && !gKnobActionLatched && (millis() - gKnobPressedAt >= kClearHoldMs)) {
        gKnobActionLatched = true;
        clearConnectionsAndBonds();
    }
}

void serviceAdvertisingRestart()
{
    bool shouldRestart = false;
    const uint32_t now = millis();

    portENTER_CRITICAL(&gStateMux);
    if (gRestartAdvertisingPending && !gState.connected && now >= gRestartAdvertisingAt) {
        gRestartAdvertisingPending = false;
        shouldRestart = true;
    }
    portEXIT_CRITICAL(&gStateMux);

    if (!shouldRestart) {
        return;
    }

    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising restarted.");

    const uint8_t bondCount = getBondCount();

    portENTER_CRITICAL(&gStateMux);
    gState.advertising = true;
    if (!gState.authFailed) {
        copyText(gState.status, sizeof(gState.status), "Advertising");
    }
    gState.bondCount = bondCount;
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);
}

class UartSecurityCallbacks final : public BLESecurityCallbacks {
public:
    uint32_t onPassKeyRequest() override
    {
        Serial.printf("[BLE] Passkey requested -> %06lu\n", static_cast<unsigned long>(kPasskey));
        updateStatus("Pairing");
        return kPasskey;
    }

    void onPassKeyNotify(uint32_t passKey) override
    {
        Serial.printf("[BLE] Passkey notify -> %06lu\n", static_cast<unsigned long>(passKey));

        portENTER_CRITICAL(&gStateMux);
        gState.passkey = passKey;
        gState.authInProgress = true;
        copyText(gState.status, sizeof(gState.status), "Pairing");
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);
    }

    bool onSecurityRequest() override
    {
        Serial.println("[BLE] Security request accepted.");

        portENTER_CRITICAL(&gStateMux);
        gState.authInProgress = true;
        copyText(gState.status, sizeof(gState.status), "Pairing");
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);

        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t authResult) override
    {
        const std::string peerAddress = BLEAddress(authResult.bd_addr).toString();
        const uint8_t bondCount = getBondCount();
        Serial.printf("[BLE] Authentication complete for %s -> success=%d fail_reason=%u\n",
                      peerAddress.c_str(),
                      authResult.success,
                      authResult.fail_reason);

        portENTER_CRITICAL(&gStateMux);
        gState.authInProgress = false;
        gState.authFailed = !authResult.success;
        gState.bonded = authResult.success;
        gState.bondCount = bondCount;
        copyText(gState.status,
                 sizeof(gState.status),
                 authResult.success ? "Bonded" : "Auth Failed");
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);
    }

    bool onConfirmPIN(uint32_t pin) override
    {
        Serial.printf("[BLE] Confirm PIN -> %06lu\n", static_cast<unsigned long>(pin));
        updateStatus("Pairing");
        return true;
    }
};

class UartServerCallbacks final : public BLEServerCallbacks {
public:
    void onConnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) override
    {
        const std::string peerAddress = BLEAddress(param->connect.remote_bda).toString();
        Serial.printf("[BLE] Connected: %s conn_id=%u\n", peerAddress.c_str(), param->connect.conn_id);

        portENTER_CRITICAL(&gStateMux);
        gState.connected = true;
        gState.advertising = false;
        gState.authInProgress = true;
        gState.authFailed = false;
        gState.connId = param->connect.conn_id;
        copyText(gState.status, sizeof(gState.status), "Pairing");
        copyText(gState.peerAddress, sizeof(gState.peerAddress), peerAddress.c_str());
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);

        server->updateConnParams(param->connect.remote_bda, 0x10, 0x20, 0, 400);
    }

    void onDisconnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) override
    {
        const uint8_t bondCount = getBondCount();
        Serial.printf("[BLE] Disconnected conn_id=%u reason=%u\n",
                      param->disconnect.conn_id,
                      param->disconnect.reason);
        (void)server;

        portENTER_CRITICAL(&gStateMux);
        gState.connected = false;
        gState.authInProgress = false;
        gState.connId = kInvalidConnId;
        gState.bondCount = bondCount;
        copyText(gState.status, sizeof(gState.status), "Disconnected");
        copyText(gState.peerAddress, sizeof(gState.peerAddress), "");
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);

        scheduleAdvertisingRestart(kAdvertisingRestartDelayMs);
    }
};

class UartRxCallbacks final : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic *characteristic) override
    {
        const std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print("[BLE] RX: ");
        Serial.write(reinterpret_cast<const uint8_t *>(rxValue.data()), rxValue.length());
        Serial.println();

        updateLastRx(rxValue);

        portENTER_CRITICAL(&gStateMux);
        ++gState.rxCount;
        if (gState.connected && !gState.authFailed && !gState.authInProgress) {
            copyText(gState.status, sizeof(gState.status), "Connected");
        }
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);

        if (gTxCharacteristic == nullptr) {
            return;
        }

        std::string reply = "Echo: ";
        reply += rxValue;
        if (reply.length() > kMaxEchoBytes) {
            reply.resize(kMaxEchoBytes);
        }

        gTxCharacteristic->setValue(reply);
        gTxCharacteristic->notify();

        Serial.print("[BLE] TX: ");
        Serial.write(reinterpret_cast<const uint8_t *>(reply.data()), reply.length());
        Serial.println();

        portENTER_CRITICAL(&gStateMux);
        ++gState.txCount;
        gDisplayDirty = true;
        portEXIT_CRITICAL(&gStateMux);
    }
};

UartSecurityCallbacks gSecurityCallbacks;
UartServerCallbacks gServerCallbacks;
UartRxCallbacks gRxCallbacks;

void initDisplay()
{
    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    gTft.begin();
    gTft.setRotation(3);
    gTft.fillScreen(TFT_BLACK);
    gTft.setTextWrap(false);

    digitalWrite(DISPLAY_BL, HIGH);
}

void initBleUart()
{
    BLEDevice::init(kDeviceName);
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
    BLEDevice::setSecurityCallbacks(&gSecurityCallbacks);

    BLESecurity security;
    security.setStaticPIN(kPasskey);
    security.setCapability(ESP_IO_CAP_OUT);
    security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
    security.setKeySize(16);
    security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    gServer = BLEDevice::createServer();
    gServer->setCallbacks(&gServerCallbacks);

    BLEService *service = gServer->createService(kServiceUuid);

    gTxCharacteristic = service->createCharacteristic(
        kTxUuid,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    gTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic *rxCharacteristic = service->createCharacteristic(
        kRxUuid,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    rxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
    rxCharacteristic->setCallbacks(&gRxCallbacks);

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(kServiceUuid);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    const uint8_t bondCount = getBondCount();
    const std::string localAddress = BLEDevice::getAddress().toString();

    portENTER_CRITICAL(&gStateMux);
    gState.connected = false;
    gState.advertising = true;
    gState.authInProgress = false;
    gState.authFailed = false;
    gState.bonded = false;
    gState.connId = kInvalidConnId;
    gState.passkey = kPasskey;
    gState.bondCount = bondCount;
    copyText(gState.status, sizeof(gState.status), "Advertising");
    copyText(gState.localAddress, sizeof(gState.localAddress), localAddress.c_str());
    copyText(gState.peerAddress, sizeof(gState.peerAddress), "");
    copyText(gState.lastRx, sizeof(gState.lastRx), "-");
    gDisplayDirty = true;
    portEXIT_CRITICAL(&gStateMux);

    Serial.printf("[BLE] Device name: %s\n", kDeviceName);
    Serial.printf("[BLE] Local MAC: %s\n", localAddress.c_str());
    Serial.printf("[BLE] Service UUID: %s\n", kServiceUuid);
    Serial.println("[BLE] Waiting for BLE central...");
}

} // namespace

void setup()
{
    boardPrepareSpi();

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);

    pinMode(ENCODER_KEY, INPUT);

    Serial.begin(115200);
    Serial.println();
    Serial.println("[APP] BLE UART pairing demo starting...");

    initDisplay();
    initBleUart();
    renderScreen();
}

void loop()
{
    serviceKnobHold();
    serviceAdvertisingRestart();

    const uint32_t now = millis();
    bool dirty = false;
    {
        AppState unused = snapshotState(&dirty);
        (void)unused;
    }
    if (dirty || (now - gLastUiDrawMs >= kUiRefreshMs)) {
        gLastUiDrawMs = now;
        renderScreen();
    }

    delay(5);
}
