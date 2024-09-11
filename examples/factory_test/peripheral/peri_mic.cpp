
#include "driver/i2s.h"
#include "utilities.h"
#include "peripheral.h"

#define EXAMPLE_REC_TIME    15       // Recording time
#define EXAMPLE_SAMPLE_RATE 44100    // Audio Sample Rate  44.1KHz
#define EXAMPLE_BIT_SAMPLE  16       // Audio Bit Sample

void init_microphone(void)
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = EXAMPLE_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
        .dma_buf_count = 8,
        .dma_buf_len = 200,
        .use_apll = 0,
    };

    // Set the pinout configuration (set using menuconfig)
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = BOARD_MIC_CLK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = BOARD_MIC_DATA,
    };

    // Call driver installation function before any I2S R/W operation.
    ESP_ERROR_CHECK( i2s_driver_install((i2s_port_t )EXAMPLE_I2S_CH, &i2s_config, 0, NULL) );
    ESP_ERROR_CHECK( i2s_set_pin((i2s_port_t)EXAMPLE_I2S_CH, &pin_config) );
    ESP_ERROR_CHECK( i2s_set_clk((i2s_port_t )EXAMPLE_I2S_CH, EXAMPLE_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO) );
}