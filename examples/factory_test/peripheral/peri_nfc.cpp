#include "peripheral.h"

#include <string.h>

Adafruit_PN532 nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);

static constexpr uint8_t NFC_UID_CACHE_CAPACITY = 10;

typedef struct {
    uint8_t uid[NFC_UID_MAX_LEN];
    uint8_t uid_len;
} nfc_uid_entry_t;

static portMUX_TYPE nfc_status_lock = portMUX_INITIALIZER_UNLOCKED;
static nfc_status_t nfc_status = {
    0,
    0,
    0,
    0,
    0,
    {0},
    0,
    NFC_EVENT_WAIT,
    false
};
static nfc_uid_entry_t nfc_seen_uids[NFC_UID_CACHE_CAPACITY] = {};
static uint8_t nfc_seen_uid_count = 0;

static bool nfc_uid_equals(const uint8_t *lhs, uint8_t lhs_len, const uint8_t *rhs, uint8_t rhs_len)
{
    return (lhs_len == rhs_len) && (lhs_len != 0) && (memcmp(lhs, rhs, lhs_len) == 0);
}

static bool nfc_uid_seen(const uint8_t *uid, uint8_t uid_len)
{
    for (uint8_t i = 0; i < nfc_seen_uid_count; ++i) {
        if (nfc_uid_equals(uid, uid_len, nfc_seen_uids[i].uid, nfc_seen_uids[i].uid_len)) {
            return true;
        }
    }
    return false;
}

static void nfc_cache_uid(const uint8_t *uid, uint8_t uid_len)
{
    if ((uid == NULL) || (uid_len == 0) || (nfc_seen_uid_count >= NFC_UID_CACHE_CAPACITY)) {
        return;
    }

    memcpy(nfc_seen_uids[nfc_seen_uid_count].uid, uid, uid_len);
    nfc_seen_uids[nfc_seen_uid_count].uid_len = uid_len;
    nfc_seen_uid_count++;
}

static void nfc_reset_runtime_state(void)
{
    memset(nfc_seen_uids, 0, sizeof(nfc_seen_uids));
    nfc_seen_uid_count = 0;

    portENTER_CRITICAL(&nfc_status_lock);
    nfc_status.scan_count = 0;
    nfc_status.unique_count = 0;
    nfc_status.last_tick_ms = 0;
    nfc_status.update_seq++;
    memset(nfc_status.last_uid, 0, sizeof(nfc_status.last_uid));
    nfc_status.last_uid_len = 0;
    nfc_status.last_event = NFC_EVENT_WAIT;
    portEXIT_CRITICAL(&nfc_status_lock);
}

static void nfc_set_init_state(bool init_ok, uint32_t version_data)
{
    portENTER_CRITICAL(&nfc_status_lock);
    nfc_status.init_flag = init_ok;
    nfc_status.version_data = version_data;
    nfc_status.last_event = init_ok ? NFC_EVENT_WAIT : NFC_EVENT_FAIL;
    nfc_status.last_tick_ms = 0;
    nfc_status.update_seq++;
    if (!init_ok) {
        memset(nfc_status.last_uid, 0, sizeof(nfc_status.last_uid));
        nfc_status.last_uid_len = 0;
    }
    portEXIT_CRITICAL(&nfc_status_lock);
}

static void nfc_update_status(int event, const uint8_t *uid, uint8_t uid_len, bool count_unique)
{
    uint32_t now = millis();

    portENTER_CRITICAL(&nfc_status_lock);
    nfc_status.scan_count++;
    if (count_unique) {
        nfc_status.unique_count++;
    }
    nfc_status.last_tick_ms = now;
    nfc_status.update_seq++;
    nfc_status.last_event = event;
    nfc_status.last_uid_len = uid_len;
    memset(nfc_status.last_uid, 0, sizeof(nfc_status.last_uid));
    if ((uid != NULL) && (uid_len != 0)) {
        memcpy(nfc_status.last_uid, uid, uid_len);
    }
    portEXIT_CRITICAL(&nfc_status_lock);
}

void nfc_init(void)
{
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("Didn't find PN53x board");
        nfc_reset_runtime_state();
        nfc_set_init_state(false, 0);
        return;
    }

    Serial.print("Found chip PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);
    Serial.println("Waiting for an ISO14443A Card ...");

    nfc_reset_runtime_state();
    nfc_set_init_state(true, versiondata);
}

bool nfc_is_init(void)
{
    bool init_flag = false;

    portENTER_CRITICAL(&nfc_status_lock);
    init_flag = nfc_status.init_flag;
    portEXIT_CRITICAL(&nfc_status_lock);
    return init_flag;
}

uint32_t nfc_get_ver_data(void)
{
    uint32_t version_data = 0;

    portENTER_CRITICAL(&nfc_status_lock);
    version_data = nfc_status.version_data;
    portEXIT_CRITICAL(&nfc_status_lock);
    return version_data;
}

void nfc_get_status(nfc_status_t *status)
{
    if (status == NULL) {
        return;
    }

    portENTER_CRITICAL(&nfc_status_lock);
    *status = nfc_status;
    portEXIT_CRITICAL(&nfc_status_lock);
}

void nfc_task(void *param)
{
    uint8_t uid[NFC_UID_MAX_LEN] = {0};
    uint8_t uid_len = 0;

    vTaskSuspend(nfc_handle);
    while (1) {
        if (!nfc_is_init()) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        uid_len = 0;
        uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uid_len);
        if (success && (uid_len > 0)) {
            uint8_t safe_uid_len = (uid_len > NFC_UID_MAX_LEN) ? NFC_UID_MAX_LEN : uid_len;
            bool is_repeat = nfc_uid_seen(uid, safe_uid_len);

            if (!is_repeat) {
                nfc_cache_uid(uid, safe_uid_len);
            }

            nfc.PrintHex(uid, safe_uid_len);
            nfc_update_status(NFC_EVENT_PASS,
                              uid,
                              safe_uid_len,
                              !is_repeat);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
