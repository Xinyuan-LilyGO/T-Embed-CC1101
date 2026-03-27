

#include "peripheral.h"

static constexpr uint32_t NRF24_AUTO_SEND_INTERVAL_MS = 1000;
static constexpr TickType_t NRF24_TASK_TX_DELAY_TICKS = pdMS_TO_TICKS(50);
static constexpr TickType_t NRF24_TASK_RX_DELAY_TICKS = pdMS_TO_TICKS(50);
static constexpr TickType_t NRF24_RX_LOCK_TIMEOUT_TICKS = pdMS_TO_TICKS(5);
static constexpr TickType_t NRF24_MODE_SWITCH_TIMEOUT_TICKS = pdMS_TO_TICKS(50);
static constexpr TickType_t NRF24_MODE_APPLY_POLL_TICKS = pdMS_TO_TICKS(10);
static constexpr uint32_t NRF24_RX_UI_SETTLE_DELAY_MS = 150;

volatile int nrf24_mode = NRF24_MODE_SEND;
bool nrf24_init_flag = false;
static volatile bool nrf24_mode_apply_pending = false;
static volatile uint32_t nrf24_mode_apply_after_ms = 0;
static volatile int nrf24_active_mode = NRF24_MODE_SEND;

nRF24 radio24 = new Module(BOARD_NRF24_CS, BOARD_NRF24_IRQ, BOARD_NRF24_CE);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

static portMUX_TYPE nrf24_status_lock = portMUX_INITIALIZER_UNLOCKED;
static nrf24_status_t nrf24_status = {
    0,
    0,
    0,
    0,
    NRF24_MODE_SEND,
    NRF24_EVENT_IDLE,
    RADIOLIB_ERR_NONE,
    false,
    "Waiting for test"
};

static void nrf24_update_status(int event, int code, const char *payload, bool count_tx, bool count_rx)
{
    uint32_t now = millis();

    portENTER_CRITICAL(&nrf24_status_lock);
    if (count_tx) {
        nrf24_status.tx_count++;
    }
    if (count_rx) {
        nrf24_status.rx_count++;
    }
    nrf24_status.last_tick_ms = now;
    nrf24_status.update_seq++;
    nrf24_status.mode = nrf24_mode;
    nrf24_status.last_event = event;
    nrf24_status.last_code = code;
    nrf24_status.init_flag = nrf24_init_flag;

    if (payload != NULL) {
        strncpy(nrf24_status.last_payload, payload, NRF24_STATUS_PAYLOAD_MAX_LEN);
        nrf24_status.last_payload[NRF24_STATUS_PAYLOAD_MAX_LEN] = '\0';
    }
    portEXIT_CRITICAL(&nrf24_status_lock);
}

static void setFlag(void)
{
    // IRQ line is not connected on this board, keep a stub for API compatibility.
}

static bool nrf24_apply_mode_now(int mode)
{
    if ((mode != NRF24_MODE_SEND) && (mode != NRF24_MODE_RECV)) {
        return false;
    }

    if (xSemaphoreTake(radioLock, NRF24_MODE_SWITCH_TIMEOUT_TICKS) != pdTRUE)
    {
        nrf24_update_status(NRF24_EVENT_ERROR,
                            RADIOLIB_ERR_NONE,
                            "NRF24 busy, mode switch timeout",
                            false,
                            false);
        return false;
    }

    board_spi_prepare_nrf24();

    byte addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
    int previous_mode = nrf24_active_mode;
    int state = radio24.finishTransmit();

    if (state != RADIOLIB_ERR_NONE) {
        nrf24_mode = previous_mode;
        nrf24_active_mode = previous_mode;
        nrf24_update_status(NRF24_EVENT_ERROR,
                            state,
                            "Failed to stop previous NRF24 mode",
                            false,
                            false);
        xSemaphoreGive(radioLock);
        return false;
    }

    if (mode == NRF24_MODE_SEND)
    {
        Serial.print(F("[nRF24] Setting transmit pipe ... "));
        state = radio24.setTransmitPipe(addr);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            nrf24_mode = previous_mode;
            nrf24_update_status(NRF24_EVENT_ERROR, state, "TX pipe setup failed", false, false);
            xSemaphoreGive(radioLock);
            return false;
        }

        if (BOARD_NRF24_IRQ >= 0) {
            radio24.setPacketSentAction(setFlag);
        }

        nrf24_mode = mode;

        Serial.print(F("[nRF24] Sending first packet ... "));
        transmissionState = radio24.startTransmit("Hello World!");
        nrf24_active_mode = mode;
        nrf24_update_status(NRF24_EVENT_MODE_CHANGE,
                            transmissionState,
                            "TX mode: auto packet enabled",
                            false,
                            false);
    }
    else
    {
        state = radio24.setReceivePipe(0, addr);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            nrf24_mode = previous_mode;
            nrf24_active_mode = previous_mode;
            nrf24_update_status(NRF24_EVENT_ERROR, state, "RX pipe setup failed", false, false);
            xSemaphoreGive(radioLock);
            return false;
        }

        if (BOARD_NRF24_IRQ >= 0) {
            radio24.setPacketReceivedAction(setFlag);
        }

        nrf24_mode = mode;

        Serial.print(F("[nRF24] Starting to listen ... "));
        state = radio24.startReceive();
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
            nrf24_active_mode = mode;
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            nrf24_mode = previous_mode;
            nrf24_active_mode = previous_mode;
            nrf24_update_status(NRF24_EVENT_ERROR, state, "RX listen start failed", false, false);
            xSemaphoreGive(radioLock);
            return false;
        }

        nrf24_update_status(NRF24_EVENT_MODE_CHANGE,
                            state,
                            "RX mode: listening on pipe 0",
                            false,
                            false);
    }

    xSemaphoreGive(radioLock);
    return true;
}

void nrf24_init(void)
{
    board_spi_prepare_nrf24();
    SPI.end();
    SPI.begin(BOARD_NRF24_SCK, BOARD_NRF24_MISO, BOARD_NRF24_MOSI);
    board_spi_prepare_nrf24();

    // initialize nRF24 with default settings
    Serial.print(F("[nRF24] Initializing ... "));
    int state = radio24.begin();
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
        nrf24_init_flag = true;
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        nrf24_init_flag = false;
        nrf24_update_status(NRF24_EVENT_ERROR, state, "Module init failed", false, false);
        return;
    }

    radio24.setFrequency(2400);

    radio24.setBitRate(1000);

    radio24.setOutputPower(0);

    // while(1){
    //     radio24.transmitDirect();
    // }

    nrf24_set_mode(nrf24_mode);
}

bool nrf24_is_init(void)
{
    return nrf24_init_flag;
}

int count = 0;

void nrf24_send(const char *str)
{
    // check if the previous transmission finished
    if (xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    board_spi_prepare_nrf24();

    if (transmissionState == RADIOLIB_ERR_NONE)
    {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

        // NOTE: when using interrupt-driven transmit method,
        //       it is not possible to automatically measure
        //       transmission data rate using getDataRate()
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio24.finishTransmit();

    // wait a second before transmitting again
    // delay(1000);

    // send another one
    Serial.print(F("[nRF24] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 32 characters long
    // String str = "Hello World! #" + String(count++);
    transmissionState = radio24.startTransmit(str);

    //
    xSemaphoreGive(radioLock);

    if (transmissionState == RADIOLIB_ERR_NONE) {
        nrf24_update_status(NRF24_EVENT_TX, transmissionState, str, true, false);
    } else {
        nrf24_update_status(NRF24_EVENT_ERROR, transmissionState, "Transmit start failed", false, false);
    }
}

static bool nrf24_recv(void)
{
    if (xSemaphoreTake(radioLock, NRF24_RX_LOCK_TIMEOUT_TICKS) != pdTRUE)
    {
        return false;
    }

    board_spi_prepare_nrf24();

    bool rx_ready = radio24.getStatus(RADIOLIB_NRF24_RX_DR);
    size_t packet_len = rx_ready ? radio24.getPacketLength() : 0;

    if (!rx_ready && (packet_len == 0))
    {
        xSemaphoreGive(radioLock);
        return false;
    }

    String str;
    int state = radio24.readData(str);
    bool recv_ok = false;

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio24.getPacketLength();
      int state = radio24.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE)
    {
        // packet was successfully received
        Serial.println(F("[nRF24] Received packet!"));
        // print data of the packet
        Serial.print(F("[nRF24] Data:\t\t"));
        Serial.println(str);
        nrf24_update_status(NRF24_EVENT_RX, state, str.c_str(), false, true);
        recv_ok = true;
    }
    else
    {
        // some other error occurred
        Serial.print(F("[nRF24] Failed, code "));
        Serial.println(state);
    }

    // put module back to listen mode
    radio24.startReceive();

    //
    xSemaphoreGive(radioLock);
    return recv_ok;
}

void nrf24_task(void *param)
{
    int tx_sequence = 0;
    uint32_t last_tx_ms = 0;

    vTaskSuspend(nrf24_handle);
    while (1)
    {
        TickType_t loop_delay = NRF24_TASK_TX_DELAY_TICKS;

        if(nrf24_is_init()) {
            if (nrf24_mode_apply_pending) {
                uint32_t now = millis();
                loop_delay = NRF24_MODE_APPLY_POLL_TICKS;
                if ((int32_t)(now - nrf24_mode_apply_after_ms) >= 0) {
                    int requested_mode = nrf24_mode;
                    if (nrf24_apply_mode_now(requested_mode) && (requested_mode == NRF24_MODE_SEND)) {
                        last_tx_ms = now;
                    }
                    nrf24_mode_apply_pending = false;
                }
            }
            else if (nrf24_mode == NRF24_MODE_SEND)
            {
                uint32_t now = millis();
                if ((now - last_tx_ms) >= NRF24_AUTO_SEND_INTERVAL_MS)
                {
                    char payload[NRF24_STATUS_PAYLOAD_MAX_LEN + 1];
                    snprintf(payload, sizeof(payload), "Hello World! #%d", tx_sequence++);
                    nrf24_send(payload);
                    ws2812_pos_demo(tx_sequence);
                    last_tx_ms = now;
                }
            }
            else if (nrf24_mode == NRF24_MODE_RECV)
            {
                loop_delay = NRF24_TASK_RX_DELAY_TICKS;
                if (nrf24_recv()) {
                    ws2812_pos_demo1();
                }
            }
        }
        vTaskDelay(loop_delay);
    }
}

int nrf24_get_mode(void)
{
    return nrf24_mode;
}

void nrf24_set_mode(int mode)
{
    if ((mode != NRF24_MODE_SEND) && (mode != NRF24_MODE_RECV)) {
        return;
    }

    if (nrf24_handle == NULL) {
        nrf24_mode = mode;
        nrf24_apply_mode_now(mode);
        return;
    }

    nrf24_mode = mode;
    nrf24_mode_apply_after_ms = millis() + ((mode == NRF24_MODE_RECV) ? NRF24_RX_UI_SETTLE_DELAY_MS : 0);
    nrf24_mode_apply_pending = true;
    nrf24_update_status(NRF24_EVENT_MODE_CHANGE,
                        RADIOLIB_ERR_NONE,
                        (mode == NRF24_MODE_RECV) ? "RX mode: refresh UI before listen" : "TX mode: preparing sender",
                        false,
                        false);
}

void nrf24_get_status(nrf24_status_t *status)
{
    if (status == NULL) {
        return;
    }

    portENTER_CRITICAL(&nrf24_status_lock);
    *status = nrf24_status;
    portEXIT_CRITICAL(&nrf24_status_lock);
}
