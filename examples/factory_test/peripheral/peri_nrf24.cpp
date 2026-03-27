

#include "peripheral.h"

#define NRF24L01_CS 44  // SPI Chip Select
#define NRF24L01_CE 43  // Chip Enable Activates RX or TX(High) mode
#define NRF24L01_IQR -1 // Maskable interrupt pin. Active low
#define NRF24L01_MOSI 9
#define NRF24L01_MISO 10
#define NRF24L01_SCK 11

static constexpr uint32_t NRF24_AUTO_SEND_INTERVAL_MS = 500;

int nrf24_mode = NRF24_MODE_SEND;
bool nrf24_init_flag = false;

nRF24 radio24 = new Module(NRF24L01_CS, NRF24L01_IQR, NRF24L01_CE);

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

void nrf24_init(void)
{
    SPI.end();
    SPI.begin(NRF24L01_SCK, NRF24L01_MISO, NRF24L01_MOSI);

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
    if (xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    if (!radio24.getStatus(RADIOLIB_NRF24_RX_DR))
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
        if(nrf24_is_init()) {
            if (nrf24_mode == NRF24_MODE_SEND)
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
                if (nrf24_recv()) {
                    ws2812_pos_demo1();
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int nrf24_get_mode(void)
{
    return nrf24_mode;
}

void nrf24_set_mode(int mode)
{
    if (xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    nrf24_mode = mode;
    if (nrf24_mode == NRF24_MODE_SEND)
    {
        // set transmit address
        // NOTE: address width in bytes MUST be equal to the
        //       width set in begin() or setAddressWidth()
        //       methods (5 by default)
        byte addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
        Serial.print(F("[nRF24] Setting transmit pipe ... "));
        int state = radio24.setTransmitPipe(addr);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true)
            {
                delay(10);
            }
        }

        // set the function that will be called
        // when packet transmission is finished
        if (NRF24L01_IQR >= 0) {
            radio24.setPacketSentAction(setFlag);
        }

        // start transmitting the first packet
        Serial.print(F("[nRF24] Sending first packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        transmissionState = radio24.startTransmit("Hello World!");
        nrf24_update_status(NRF24_EVENT_MODE_CHANGE,
                            transmissionState,
                            "TX mode: auto packet enabled",
                            false,
                            false);
    }
    else
    {
        byte addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
        int state = radio24.setReceivePipe(0, addr);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true)
                ;
        }

        // set the function that will be called
        // when new packet is received
        if (NRF24L01_IQR >= 0) {
            radio24.setPacketReceivedAction(setFlag);
        }

        // start listening
        Serial.print(F("[nRF24] Starting to listen ... "));
        state = radio24.startReceive();
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true)
                ;
        }

        nrf24_update_status(NRF24_EVENT_MODE_CHANGE,
                            state,
                            "RX mode: listening on pipe 0",
                            false,
                            false);
    }

    xSemaphoreGive(radioLock);
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
