#include "peripheral.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <esp_gap_ble_api.h>

#include <cstring>
#include <string>

namespace {

constexpr char kBleUartDeviceName[] = "T-Embed-BLE-UART";
constexpr uint32_t kBleUartPasskey = 123456;

constexpr char kBleUartServiceUuid[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char kBleUartRxUuid[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char kBleUartTxUuid[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

constexpr uint16_t kBleUartInvalidConnId = 0xFFFF;
constexpr uint32_t kBleUartAdvertisingRestartDelayMs = 500;
constexpr uint32_t kBleUartClearRestartDelayMs = 200;
constexpr uint32_t kBleUartMaxEchoBytes = 120;

portMUX_TYPE ble_uart_status_lock = portMUX_INITIALIZER_UNLOCKED;

BLEServer *ble_uart_server = nullptr;
BLECharacteristic *ble_uart_tx_characteristic = nullptr;

volatile bool ble_uart_restart_advertising_pending = false;
volatile uint32_t ble_uart_restart_advertising_at = 0;
uint16_t ble_uart_conn_id = kBleUartInvalidConnId;

ble_uart_status_t ble_uart_status = {};

void ble_uart_copy_text(char *dest, size_t dest_size, const char *value)
{
    if (dest_size == 0) {
        return;
    }
    snprintf(dest, dest_size, "%s", value ? value : "");
}

void ble_uart_format_payload(const std::string &value, char *dest, size_t dest_size)
{
    if (dest_size == 0) {
        return;
    }

    size_t write_index = 0;
    for (char ch : value) {
        if (write_index >= (dest_size - 1)) {
            break;
        }

        unsigned char uch = static_cast<unsigned char>(ch);
        if ((uch == '\r') || (uch == '\n') || (uch == '\t')) {
            dest[write_index++] = ' ';
        } else if ((uch >= 32) && (uch < 127)) {
            dest[write_index++] = ch;
        } else {
            dest[write_index++] = '?';
        }
    }

    if (write_index == 0) {
        dest[write_index++] = '-';
    }
    dest[write_index] = '\0';
}

uint8_t ble_uart_get_bond_count_internal()
{
    int count = esp_ble_get_bond_device_num();
    if (count <= 0) {
        return 0;
    }
    return static_cast<uint8_t>(count > 255 ? 255 : count);
}

void ble_uart_touch_locked(uint32_t now)
{
    ble_uart_status.last_tick_ms = now;
    ble_uart_status.update_seq++;
}

void ble_uart_schedule_advertising_restart(uint32_t delay_ms)
{
    portENTER_CRITICAL(&ble_uart_status_lock);
    ble_uart_restart_advertising_pending = true;
    ble_uart_restart_advertising_at = millis() + delay_ms;
    portEXIT_CRITICAL(&ble_uart_status_lock);
}

bool ble_uart_initialized_locked()
{
    return ble_uart_status.init_flag;
}

void ble_uart_reset_runtime_status_locked(uint8_t bond_count)
{
    ble_uart_status.connected = false;
    ble_uart_status.advertising = false;
    ble_uart_status.auth_in_progress = false;
    ble_uart_status.bonded = (bond_count > 0);
    ble_uart_status.bond_count = bond_count;
    ble_uart_conn_id = kBleUartInvalidConnId;
    ble_uart_copy_text(ble_uart_status.peer_address, sizeof(ble_uart_status.peer_address), "");
}

bool ble_uart_clear_bonded_devices_internal()
{
    int bond_count = esp_ble_get_bond_device_num();
    if (bond_count <= 0) {
        return true;
    }

    esp_ble_bond_dev_t *bonded_devices = static_cast<esp_ble_bond_dev_t *>(
        malloc(sizeof(esp_ble_bond_dev_t) * static_cast<size_t>(bond_count)));
    if (bonded_devices == nullptr) {
        Serial.println("[BLE] Failed to allocate bonded device list.");
        return false;
    }

    int list_count = bond_count;
    esp_err_t list_result = esp_ble_get_bond_device_list(&list_count, bonded_devices);
    if (list_result != ESP_OK) {
        Serial.printf("[BLE] Failed to get bond list: %d\n", list_result);
        free(bonded_devices);
        return false;
    }

    for (int index = 0; index < list_count; ++index) {
        char address[BLE_UART_STATUS_ADDRESS_MAX_LEN];
        BLEAddress ble_address(bonded_devices[index].bd_addr);
        ble_uart_copy_text(address, sizeof(address), ble_address.toString().c_str());
        esp_err_t result = esp_ble_remove_bond_device(bonded_devices[index].bd_addr);
        Serial.printf("[BLE] Remove bond %s -> %d\n", address, result);
    }

    free(bonded_devices);
    delay(100);
    return true;
}

class BleUartSecurityCallbacks final : public BLESecurityCallbacks {
public:
    uint32_t onPassKeyRequest() override
    {
        uint32_t now = millis();
        Serial.printf("[BLE] Passkey requested -> %06lu\n", static_cast<unsigned long>(kBleUartPasskey));

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_status.passkey = kBleUartPasskey;
        ble_uart_status.auth_in_progress = true;
        ble_uart_status.last_event = BLE_UART_EVENT_PAIRING;
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);
        return kBleUartPasskey;
    }

    void onPassKeyNotify(uint32_t pass_key) override
    {
        uint32_t now = millis();
        Serial.printf("[BLE] Passkey notify -> %06lu\n", static_cast<unsigned long>(pass_key));

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_status.passkey = pass_key;
        ble_uart_status.auth_in_progress = true;
        ble_uart_status.last_event = BLE_UART_EVENT_PAIRING;
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);
    }

    bool onSecurityRequest() override
    {
        uint32_t now = millis();
        Serial.println("[BLE] Security request accepted.");

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_status.auth_in_progress = true;
        ble_uart_status.last_event = BLE_UART_EVENT_PAIRING;
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_result) override
    {
        uint32_t now = millis();
        std::string peer_address = BLEAddress(auth_result.bd_addr).toString();
        uint8_t bond_count = ble_uart_get_bond_count_internal();

        Serial.printf("[BLE] Authentication complete for %s -> success=%d fail_reason=%u\n",
                      peer_address.c_str(),
                      auth_result.success,
                      auth_result.fail_reason);

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_status.auth_in_progress = false;
        ble_uart_status.bonded = auth_result.success;
        ble_uart_status.bond_count = bond_count;
        ble_uart_status.last_event = auth_result.success ? BLE_UART_EVENT_BONDED : BLE_UART_EVENT_AUTH_FAILED;
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);
    }

    bool onConfirmPIN(uint32_t pin) override
    {
        uint32_t now = millis();
        Serial.printf("[BLE] Confirm PIN -> %06lu\n", static_cast<unsigned long>(pin));

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_status.auth_in_progress = true;
        ble_uart_status.last_event = BLE_UART_EVENT_PAIRING;
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);
        return true;
    }
};

class BleUartServerCallbacks final : public BLEServerCallbacks {
public:
    void onConnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) override
    {
        uint32_t now = millis();
        std::string peer_address = BLEAddress(param->connect.remote_bda).toString();
        Serial.printf("[BLE] Connected: %s conn_id=%u\n", peer_address.c_str(), param->connect.conn_id);

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_conn_id = param->connect.conn_id;
        ble_uart_status.connected = true;
        ble_uart_status.advertising = false;
        ble_uart_status.auth_in_progress = true;
        ble_uart_status.last_event = BLE_UART_EVENT_PAIRING;
        ble_uart_copy_text(ble_uart_status.peer_address, sizeof(ble_uart_status.peer_address), peer_address.c_str());
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);

        server->updateConnParams(param->connect.remote_bda, 0x10, 0x20, 0, 400);
    }

    void onDisconnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) override
    {
        uint32_t now = millis();
        uint8_t bond_count = ble_uart_get_bond_count_internal();
        Serial.printf("[BLE] Disconnected conn_id=%u reason=%u\n",
                      param->disconnect.conn_id,
                      param->disconnect.reason);
        (void)server;

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_reset_runtime_status_locked(bond_count);
        ble_uart_status.last_event = BLE_UART_EVENT_DISCONNECTED;
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);

        ble_uart_schedule_advertising_restart(kBleUartAdvertisingRestartDelayMs);
    }
};

class BleUartRxCallbacks final : public BLECharacteristicCallbacks {
public:
    void onWrite(BLECharacteristic *characteristic) override
    {
        std::string rx_value = characteristic->getValue();
        if (rx_value.empty()) {
            return;
        }

        Serial.print("[BLE] RX: ");
        Serial.write(reinterpret_cast<const uint8_t *>(rx_value.data()), rx_value.length());
        Serial.println();

        std::string reply = "Echo: ";
        reply += rx_value;
        if (reply.length() > kBleUartMaxEchoBytes) {
            reply.resize(kBleUartMaxEchoBytes);
        }

        uint32_t now = millis();
        char payload[BLE_UART_STATUS_PAYLOAD_MAX_LEN + 1];
        ble_uart_format_payload(rx_value, payload, sizeof(payload));

        portENTER_CRITICAL(&ble_uart_status_lock);
        ble_uart_status.rx_count++;
        ble_uart_status.last_event = BLE_UART_EVENT_CONNECTED;
        ble_uart_copy_text(ble_uart_status.last_payload, sizeof(ble_uart_status.last_payload), payload);
        ble_uart_touch_locked(now);
        portEXIT_CRITICAL(&ble_uart_status_lock);

        if (ble_uart_tx_characteristic != nullptr) {
            ble_uart_tx_characteristic->setValue(reply);
            ble_uart_tx_characteristic->notify();

            Serial.print("[BLE] TX: ");
            Serial.write(reinterpret_cast<const uint8_t *>(reply.data()), reply.length());
            Serial.println();

            portENTER_CRITICAL(&ble_uart_status_lock);
            ble_uart_status.tx_count++;
            ble_uart_touch_locked(now);
            portEXIT_CRITICAL(&ble_uart_status_lock);
        }
    }
};

BleUartSecurityCallbacks ble_uart_security_callbacks;
BleUartServerCallbacks ble_uart_server_callbacks;
BleUartRxCallbacks ble_uart_rx_callbacks;

}  // namespace

void ble_uart_init(void)
{
    portENTER_CRITICAL(&ble_uart_status_lock);
    if (ble_uart_initialized_locked()) {
        portEXIT_CRITICAL(&ble_uart_status_lock);
        return;
    }
    portEXIT_CRITICAL(&ble_uart_status_lock);

    BLEDevice::init(kBleUartDeviceName);
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
    BLEDevice::setSecurityCallbacks(&ble_uart_security_callbacks);

    BLESecurity security;
    security.setStaticPIN(kBleUartPasskey);
    security.setCapability(ESP_IO_CAP_OUT);
    security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
    security.setKeySize(16);
    security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    ble_uart_server = BLEDevice::createServer();
    ble_uart_server->setCallbacks(&ble_uart_server_callbacks);

    BLEService *service = ble_uart_server->createService(kBleUartServiceUuid);

    ble_uart_tx_characteristic = service->createCharacteristic(
        kBleUartTxUuid,
        BLECharacteristic::PROPERTY_NOTIFY);
    ble_uart_tx_characteristic->addDescriptor(new BLE2902());

    BLECharacteristic *rx_characteristic = service->createCharacteristic(
        kBleUartRxUuid,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
    rx_characteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
    rx_characteristic->setCallbacks(&ble_uart_rx_callbacks);

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(kBleUartServiceUuid);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    uint32_t now = millis();
    uint8_t bond_count = ble_uart_get_bond_count_internal();
    std::string local_address = BLEDevice::getAddress().toString();

    portENTER_CRITICAL(&ble_uart_status_lock);
    memset(&ble_uart_status, 0, sizeof(ble_uart_status));
    ble_uart_status.init_flag = true;
    ble_uart_status.connected = false;
    ble_uart_status.advertising = true;
    ble_uart_status.auth_in_progress = false;
    ble_uart_status.bonded = (bond_count > 0);
    ble_uart_status.bond_count = bond_count;
    ble_uart_status.passkey = kBleUartPasskey;
    ble_uart_status.last_event = BLE_UART_EVENT_ADVERTISING;
    ble_uart_copy_text(ble_uart_status.local_address, sizeof(ble_uart_status.local_address), local_address.c_str());
    ble_uart_copy_text(ble_uart_status.peer_address, sizeof(ble_uart_status.peer_address), "");
    ble_uart_copy_text(ble_uart_status.last_payload, sizeof(ble_uart_status.last_payload), "-");
    ble_uart_touch_locked(now);
    portEXIT_CRITICAL(&ble_uart_status_lock);

    Serial.printf("[BLE] Device name: %s\n", kBleUartDeviceName);
    Serial.printf("[BLE] Local MAC: %s\n", local_address.c_str());
    Serial.printf("[BLE] Service UUID: %s\n", kBleUartServiceUuid);
    Serial.println("[BLE] Waiting for BLE central...");
}

void ble_uart_service(void)
{
    bool should_restart = false;

    portENTER_CRITICAL(&ble_uart_status_lock);
    bool initialized = ble_uart_initialized_locked();
    uint32_t now = millis();
    if (initialized && ble_uart_restart_advertising_pending && !ble_uart_status.connected &&
        (now >= ble_uart_restart_advertising_at)) {
        ble_uart_restart_advertising_pending = false;
        should_restart = true;
    }
    portEXIT_CRITICAL(&ble_uart_status_lock);

    if (!should_restart) {
        return;
    }

    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising restarted.");

    uint32_t restart_now = millis();
    uint8_t bond_count = ble_uart_get_bond_count_internal();

    portENTER_CRITICAL(&ble_uart_status_lock);
    ble_uart_status.advertising = true;
    ble_uart_status.auth_in_progress = false;
    ble_uart_status.bonded = (bond_count > 0);
    ble_uart_status.bond_count = bond_count;
    ble_uart_status.last_event = BLE_UART_EVENT_ADVERTISING;
    ble_uart_touch_locked(restart_now);
    portEXIT_CRITICAL(&ble_uart_status_lock);
}

bool ble_uart_is_init(void)
{
    bool initialized = false;
    portENTER_CRITICAL(&ble_uart_status_lock);
    initialized = ble_uart_initialized_locked();
    portEXIT_CRITICAL(&ble_uart_status_lock);
    return initialized;
}

bool ble_uart_clear_bonds(void)
{
    uint16_t conn_id = kBleUartInvalidConnId;
    bool connected = false;

    portENTER_CRITICAL(&ble_uart_status_lock);
    if (!ble_uart_initialized_locked()) {
        portEXIT_CRITICAL(&ble_uart_status_lock);
        return false;
    }

    connected = ble_uart_status.connected;
    conn_id = ble_uart_conn_id;
    ble_uart_status.advertising = false;
    ble_uart_status.auth_in_progress = false;
    ble_uart_status.last_event = BLE_UART_EVENT_CLEARING_BONDS;
    ble_uart_touch_locked(millis());
    portEXIT_CRITICAL(&ble_uart_status_lock);

    if (connected && (ble_uart_server != nullptr) && (conn_id != kBleUartInvalidConnId)) {
        Serial.printf("[BLE] Disconnecting conn_id=%u before clearing bonds.\n", conn_id);
        ble_uart_server->disconnect(conn_id);
        delay(150);
    }

    bool cleared = ble_uart_clear_bonded_devices_internal();
    uint8_t bond_count = ble_uart_get_bond_count_internal();
    uint32_t now = millis();

    portENTER_CRITICAL(&ble_uart_status_lock);
    ble_uart_reset_runtime_status_locked(bond_count);
    ble_uart_status.last_event = BLE_UART_EVENT_CLEARING_BONDS;
    ble_uart_touch_locked(now);
    portEXIT_CRITICAL(&ble_uart_status_lock);

    ble_uart_schedule_advertising_restart(kBleUartClearRestartDelayMs);
    return cleared;
}

void ble_uart_get_status(ble_uart_status_t *status)
{
    if (status == nullptr) {
        return;
    }

    portENTER_CRITICAL(&ble_uart_status_lock);
    *status = ble_uart_status;
    portEXIT_CRITICAL(&ble_uart_status_lock);
}
