#include "ble_uart.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <esp_gap_ble_api.h>

#include <cstring>
#include <string>

namespace factory_ble {

namespace {

constexpr char kDeviceName[] = "T-Embed-BLE-UART";
constexpr uint32_t kPasskey = 123456;

constexpr char kServiceUuid[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char kRxUuid[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char kTxUuid[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

constexpr uint16_t kInvalidConnId = 0xFFFF;
constexpr uint32_t kAdvertisingRestartDelayMs = 500;
constexpr uint32_t kClearRestartDelayMs = 200;
constexpr size_t kMaxEchoBytes = 120;

portMUX_TYPE gStatusLock = portMUX_INITIALIZER_UNLOCKED;

BLEServer* gServer = nullptr;
BLECharacteristic* gTxCharacteristic = nullptr;
bool gServiceCreated = false;
bool gAdvertisingConfigured = false;
bool gPageActive = false;
bool gRestartAdvertisingPending = false;
uint32_t gRestartAdvertisingAt = 0;
uint16_t gConnId = kInvalidConnId;

Status gStatus = {};

void copyText(char* destination, const size_t destinationSize, const char* value)
{
    if (destinationSize == 0) {
        return;
    }
    snprintf(destination, destinationSize, "%s", value ? value : "");
}

void formatPayload(const std::string& value, char* destination, const size_t destinationSize)
{
    if (destinationSize == 0) {
        return;
    }

    size_t writeIndex = 0;
    for (const char ch : value) {
        if (writeIndex >= destinationSize - 1) {
            break;
        }

        const unsigned char uch = static_cast<unsigned char>(ch);
        if (uch == '\r' || uch == '\n' || uch == '\t') {
            destination[writeIndex++] = ' ';
        } else if (uch >= 32 && uch < 127) {
            destination[writeIndex++] = ch;
        } else {
            destination[writeIndex++] = '?';
        }
    }

    if (writeIndex == 0) {
        destination[writeIndex++] = '-';
    }
    destination[writeIndex] = '\0';
}

uint8_t bondCount()
{
    const int count = esp_ble_get_bond_device_num();
    if (count <= 0) {
        return 0;
    }
    return static_cast<uint8_t>(count > 255 ? 255 : count);
}

void touchStatusLocked(const uint32_t now)
{
    gStatus.lastTickMs = now;
    gStatus.updateSeq++;
}

void scheduleAdvertisingRestart(const uint32_t delayMs)
{
    portENTER_CRITICAL(&gStatusLock);
    if (gPageActive) {
        gRestartAdvertisingPending = true;
        gRestartAdvertisingAt = millis() + delayMs;
    }
    portEXIT_CRITICAL(&gStatusLock);
}

void resetConnectionStatusLocked(const uint8_t currentBondCount)
{
    gStatus.connected = false;
    gStatus.advertising = false;
    gStatus.authInProgress = false;
    gStatus.bonded = currentBondCount > 0;
    gStatus.bondCount = currentBondCount;
    gConnId = kInvalidConnId;
    copyText(gStatus.peerAddress, sizeof(gStatus.peerAddress), "");
}

bool removeAllBonds()
{
    const int count = esp_ble_get_bond_device_num();
    if (count <= 0) {
        return true;
    }

    auto* devices = static_cast<esp_ble_bond_dev_t*>(
        malloc(sizeof(esp_ble_bond_dev_t) * static_cast<size_t>(count)));
    if (devices == nullptr) {
        Serial.println(F("[BLE] Failed to allocate bonded device list."));
        return false;
    }

    int listCount = count;
    const esp_err_t listResult = esp_ble_get_bond_device_list(&listCount, devices);
    if (listResult != ESP_OK) {
        Serial.printf("[BLE] Failed to get bond list: %d\n", listResult);
        free(devices);
        return false;
    }

    bool success = true;
    for (int index = 0; index < listCount; ++index) {
        char address[kAddressTextLength];
        BLEAddress bleAddress(devices[index].bd_addr);
        copyText(address, sizeof(address), bleAddress.toString().c_str());
        const esp_err_t result = esp_ble_remove_bond_device(devices[index].bd_addr);
        Serial.printf("[BLE] Remove bond %s -> %d\n", address, result);
        success = success && (result == ESP_OK);
    }

    free(devices);
    return success;
}

class SecurityCallbacks final : public BLESecurityCallbacks {
public:
    uint32_t onPassKeyRequest() override
    {
        Serial.printf("[BLE] Passkey requested -> %06lu\n", static_cast<unsigned long>(kPasskey));
        portENTER_CRITICAL(&gStatusLock);
        gStatus.passkey = kPasskey;
        gStatus.authInProgress = true;
        gStatus.lastEvent = EventPairing;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
        return kPasskey;
    }

    void onPassKeyNotify(const uint32_t passKey) override
    {
        Serial.printf("[BLE] Passkey notify -> %06lu\n", static_cast<unsigned long>(passKey));
        portENTER_CRITICAL(&gStatusLock);
        gStatus.passkey = passKey;
        gStatus.authInProgress = true;
        gStatus.lastEvent = EventPairing;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
    }

    bool onSecurityRequest() override
    {
        Serial.println(F("[BLE] Security request accepted."));
        portENTER_CRITICAL(&gStatusLock);
        gStatus.authInProgress = true;
        gStatus.lastEvent = EventPairing;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
        return true;
    }

    void onAuthenticationComplete(const esp_ble_auth_cmpl_t authResult) override
    {
        esp_bd_addr_t peerBdAddr;
        memcpy(peerBdAddr, authResult.bd_addr, sizeof(peerBdAddr));
        const std::string peerAddress = BLEAddress(peerBdAddr).toString();
        const uint8_t currentBondCount = bondCount();
        Serial.printf("[BLE] Authentication complete for %s -> success=%d fail_reason=%u\n",
                      peerAddress.c_str(),
                      authResult.success,
                      authResult.fail_reason);

        portENTER_CRITICAL(&gStatusLock);
        gStatus.authInProgress = false;
        gStatus.bonded = authResult.success;
        gStatus.bondCount = currentBondCount;
        gStatus.lastEvent = authResult.success ? EventBonded : EventAuthFailed;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
    }

    bool onConfirmPIN(const uint32_t pin) override
    {
        Serial.printf("[BLE] Confirm PIN -> %06lu\n", static_cast<unsigned long>(pin));
        portENTER_CRITICAL(&gStatusLock);
        gStatus.authInProgress = true;
        gStatus.lastEvent = EventPairing;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
        return true;
    }
};

class ServerCallbacks final : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* server, esp_ble_gatts_cb_param_t* param) override
    {
        const std::string peerAddress = BLEAddress(param->connect.remote_bda).toString();
        Serial.printf("[BLE] Connected: %s conn_id=%u\n",
                      peerAddress.c_str(),
                      param->connect.conn_id);

        bool active = false;
        portENTER_CRITICAL(&gStatusLock);
        active = gPageActive;
        gConnId = param->connect.conn_id;
        gStatus.connected = true;
        gStatus.advertising = false;
        gStatus.authInProgress = true;
        gStatus.lastEvent = EventPairing;
        copyText(gStatus.peerAddress, sizeof(gStatus.peerAddress), peerAddress.c_str());
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);

        if (!active) {
            server->disconnect(param->connect.conn_id);
            return;
        }

        server->updateConnParams(param->connect.remote_bda, 0x10, 0x20, 0, 400);
    }

    void onDisconnect(BLEServer* server, esp_ble_gatts_cb_param_t* param) override
    {
        (void)server;
        const uint8_t currentBondCount = bondCount();
        Serial.printf("[BLE] Disconnected conn_id=%u reason=%u\n",
                      param->disconnect.conn_id,
                      param->disconnect.reason);

        bool active = false;
        portENTER_CRITICAL(&gStatusLock);
        active = gPageActive;
        resetConnectionStatusLocked(currentBondCount);
        gStatus.lastEvent = EventDisconnected;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);

        if (active) {
            scheduleAdvertisingRestart(kAdvertisingRestartDelayMs);
        }
    }
};

class RxCallbacks final : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic* characteristic) override
    {
        const std::string rxValue = characteristic->getValue();
        if (rxValue.empty()) {
            return;
        }

        Serial.print(F("[BLE] RX: "));
        Serial.write(reinterpret_cast<const uint8_t*>(rxValue.data()), rxValue.length());
        Serial.println();

        char payload[kPayloadTextLength];
        formatPayload(rxValue, payload, sizeof(payload));

        portENTER_CRITICAL(&gStatusLock);
        gStatus.rxCount++;
        gStatus.lastEvent = EventConnected;
        copyText(gStatus.lastPayload, sizeof(gStatus.lastPayload), payload);
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);

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

        Serial.print(F("[BLE] TX: "));
        Serial.write(reinterpret_cast<const uint8_t*>(reply.data()), reply.length());
        Serial.println();

        portENTER_CRITICAL(&gStatusLock);
        gStatus.txCount++;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
    }
};

SecurityCallbacks gSecurityCallbacks;
ServerCallbacks gServerCallbacks;
RxCallbacks gRxCallbacks;

void initializeGattServer()
{
    if (gServiceCreated) {
        return;
    }

    gServer = BLEDevice::createServer();
    if (gServer == nullptr) {
        Serial.println(F("[BLE] Failed to create GATT server."));
        return;
    }
    gServer->setCallbacks(&gServerCallbacks);

    BLEService* service = gServer->createService(kServiceUuid);
    if (service == nullptr) {
        Serial.println(F("[BLE] Failed to create UART service."));
        return;
    }

    gTxCharacteristic = service->createCharacteristic(
        kTxUuid,
        BLECharacteristic::PROPERTY_NOTIFY);
    gTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic* rxCharacteristic = service->createCharacteristic(
        kRxUuid,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    rxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
    rxCharacteristic->setCallbacks(&gRxCallbacks);

    service->start();
    gServiceCreated = true;
}

void updateAdvertisingStatus(const bool advertising, const uint8_t currentBondCount)
{
    portENTER_CRITICAL(&gStatusLock);
    gStatus.advertising = advertising;
    gStatus.bonded = currentBondCount > 0;
    gStatus.bondCount = currentBondCount;
    if (advertising) {
        gStatus.authInProgress = false;
        gStatus.lastEvent = EventAdvertising;
    }
    touchStatusLocked(millis());
    portEXIT_CRITICAL(&gStatusLock);
}

}  // namespace

void begin()
{
    portENTER_CRITICAL(&gStatusLock);
    gPageActive = true;
    gRestartAdvertisingPending = false;
    portEXIT_CRITICAL(&gStatusLock);

    if (!BLEDevice::getInitialized()) {
        BLEDevice::init(kDeviceName);
        BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
        BLEDevice::setSecurityCallbacks(&gSecurityCallbacks);

        BLESecurity security;
        security.setStaticPIN(kPasskey);
        // The display shows the passkey, while KBDISP also lets peers choose
        // passkey entry or numeric confirmation during Secure Connections.
        security.setCapability(ESP_IO_CAP_KBDISP);
        security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
        security.setKeySize(16);
        security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    }

    initializeGattServer();
    if (!gServiceCreated) {
        portENTER_CRITICAL(&gStatusLock);
        gStatus.initialized = false;
        gStatus.advertising = false;
        gStatus.lastEvent = EventAuthFailed;
        touchStatusLocked(millis());
        portEXIT_CRITICAL(&gStatusLock);
        return;
    }

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    if (!gAdvertisingConfigured) {
        advertising->addServiceUUID(kServiceUuid);
        advertising->setScanResponse(true);
        advertising->setMinPreferred(0x06);
        advertising->setMaxPreferred(0x12);
        gAdvertisingConfigured = true;
    }

    const uint8_t currentBondCount = bondCount();
    const std::string localAddress = BLEDevice::getAddress().toString();
    portENTER_CRITICAL(&gStatusLock);
    memset(&gStatus, 0, sizeof(gStatus));
    gStatus.initialized = true;
    gStatus.advertising = false;
    gStatus.bonded = currentBondCount > 0;
    gStatus.bondCount = currentBondCount;
    gStatus.passkey = kPasskey;
    gStatus.lastEvent = EventIdle;
    copyText(gStatus.localAddress, sizeof(gStatus.localAddress), localAddress.c_str());
    copyText(gStatus.peerAddress, sizeof(gStatus.peerAddress), "");
    copyText(gStatus.lastPayload, sizeof(gStatus.lastPayload), "-");
    touchStatusLocked(millis());
    portEXIT_CRITICAL(&gStatusLock);

    BLEDevice::startAdvertising();
    updateAdvertisingStatus(true, currentBondCount);

    Serial.printf("[BLE] Device name: %s\n", kDeviceName);
    Serial.printf("[BLE] Local MAC: %s\n", localAddress.c_str());
    Serial.printf("[BLE] Service UUID: %s\n", kServiceUuid);
    Serial.println(F("[BLE] Waiting for BLE central..."));
}

void service()
{
    bool shouldRestart = false;
    portENTER_CRITICAL(&gStatusLock);
    const uint32_t now = millis();
    if (gPageActive && gRestartAdvertisingPending && !gStatus.connected && now >= gRestartAdvertisingAt) {
        gRestartAdvertisingPending = false;
        shouldRestart = true;
    }
    portEXIT_CRITICAL(&gStatusLock);

    if (!shouldRestart) {
        return;
    }

    BLEDevice::startAdvertising();
    updateAdvertisingStatus(true, bondCount());
    Serial.println(F("[BLE] Advertising restarted."));
}

void stop()
{
    uint16_t connId = kInvalidConnId;
    bool connected = false;
    bool advertising = false;
    portENTER_CRITICAL(&gStatusLock);
    gPageActive = false;
    gRestartAdvertisingPending = false;
    connected = gStatus.connected;
    advertising = gStatus.advertising;
    connId = gConnId;
    gStatus.advertising = false;
    gStatus.authInProgress = false;
    touchStatusLocked(millis());
    portEXIT_CRITICAL(&gStatusLock);

    if (advertising && BLEDevice::getInitialized()) {
        BLEDevice::stopAdvertising();
    }
    if (connected && gServer != nullptr && connId != kInvalidConnId) {
        Serial.printf("[BLE] Disconnecting conn_id=%u on page exit.\n", connId);
        gServer->disconnect(connId);
    }
}

bool clearBonds()
{
    uint16_t connId = kInvalidConnId;
    bool connected = false;
    bool advertising = false;
    portENTER_CRITICAL(&gStatusLock);
    if (!gPageActive || !gStatus.initialized) {
        portEXIT_CRITICAL(&gStatusLock);
        return false;
    }
    connected = gStatus.connected;
    advertising = gStatus.advertising;
    connId = gConnId;
    gStatus.advertising = false;
    gStatus.authInProgress = false;
    gStatus.lastEvent = EventClearingBonds;
    touchStatusLocked(millis());
    portEXIT_CRITICAL(&gStatusLock);

    if (advertising) {
        BLEDevice::stopAdvertising();
    }
    if (connected && gServer != nullptr && connId != kInvalidConnId) {
        Serial.printf("[BLE] Disconnecting conn_id=%u before clearing bonds.\n", connId);
        gServer->disconnect(connId);
        delay(150);
    }

    const bool success = removeAllBonds();
    const uint8_t currentBondCount = bondCount();
    portENTER_CRITICAL(&gStatusLock);
    resetConnectionStatusLocked(currentBondCount);
    gStatus.lastEvent = EventClearingBonds;
    touchStatusLocked(millis());
    portEXIT_CRITICAL(&gStatusLock);

    scheduleAdvertisingRestart(kClearRestartDelayMs);
    return success;
}

bool restartAdvertising()
{
    bool connected = false;
    bool advertising = false;
    portENTER_CRITICAL(&gStatusLock);
    if (!gPageActive || !gStatus.initialized) {
        portEXIT_CRITICAL(&gStatusLock);
        return false;
    }
    connected = gStatus.connected;
    advertising = gStatus.advertising;
    gRestartAdvertisingPending = false;
    portEXIT_CRITICAL(&gStatusLock);

    if (connected || advertising) {
        return false;
    }

    BLEDevice::startAdvertising();
    updateAdvertisingStatus(true, bondCount());
    Serial.println(F("[BLE] Advertising manually restarted."));
    return true;
}

void getStatus(Status* status)
{
    if (status == nullptr) {
        return;
    }
    portENTER_CRITICAL(&gStatusLock);
    *status = gStatus;
    portEXIT_CRITICAL(&gStatusLock);
}

}  // namespace factory_ble
