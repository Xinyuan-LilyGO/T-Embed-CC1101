#include "peripheral.h"
#include <sys/stat.h>
#include "driver/i2s.h"
#include "utilities.h"
#include "esp_err.h"

#define EXAMPLE_REC_TIME    15       // Recording time
#define EXAMPLE_I2S_CH      0        // I2S Channel Number
#define EXAMPLE_SAMPLE_RATE 44100    // Audio Sample Rate  44.1KHz
#define EXAMPLE_BIT_SAMPLE  16       // Audio Bit Sample

#define SPI_DMA_CHAN        SPI_DMA_CH_AUTO
#define NUM_CHANNELS        (1) // For mono recording only!
#define SAMPLE_SIZE         (EXAMPLE_BIT_SAMPLE * 1024)
#define BYTE_RATE           (EXAMPLE_SAMPLE_RATE * (EXAMPLE_BIT_SAMPLE / 8)) * NUM_CHANNELS

static int16_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;

bool recode_start_flag = false;
int recode_cnt = 0;

static portMUX_TYPE mic_spinlock = portMUX_INITIALIZER_UNLOCKED;

void generate_wav_header(char* wav_header, uint32_t wav_size, uint32_t sample_rate)
{
    // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
    uint32_t file_size = wav_size + WAVE_HEADER_SIZE - 8;
    uint32_t byte_rate = BYTE_RATE;

    const char set_wav_header[] = {
        'R','I','F','F', // ChunkID
        (char)file_size, (char)(file_size >> 8), (char)(file_size >> 16), (char)(file_size >> 24), // ChunkSize
        'W','A','V','E', // Format
        'f','m','t',' ', // Subchunk1ID
        0x10, 0x00, 0x00, 0x00, // Subchunk1Size (16 for PCM)
        0x01, 0x00, // AudioFormat (1 for PCM)
        0x01, 0x00, // NumChannels (1 channel)
        (char)sample_rate, (char)(sample_rate >> 8), (char)(sample_rate >> 16), (char)(sample_rate >> 24), // SampleRate
        (char)byte_rate, (char)(byte_rate >> 8), (char)(byte_rate >> 16), (char)(byte_rate >> 24), // ByteRate
        0x02, 0x00, // BlockAlign
        0x10, 0x00, // BitsPerSample (16 bits)
        'd','a','t','a', // Subchunk2ID
        (char)wav_size, (char)(wav_size >> 8), (char)(wav_size >> 16), (char)(wav_size >> 24), // Subchunk2Size
    };

    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

void record_wav(uint32_t rec_time)
{
    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    Serial.println("Opening File");

    char wav_header_fmt[WAVE_HEADER_SIZE];

    uint32_t flash_rec_time = BYTE_RATE * rec_time;
    generate_wav_header(wav_header_fmt, flash_rec_time, EXAMPLE_SAMPLE_RATE);

    // Create new WAV file
    char buf[32];
    snprintf(buf, 32, "/record%02d.wav", recode_cnt);
    recode_cnt++;

    File file = SD.open(buf, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        file.close();
        mic_recode_stop();
        return;
    }

    file.write((uint8_t *)wav_header_fmt, WAVE_HEADER_SIZE);

    // Start recording
    while(flash_wr_size < flash_rec_time) {
        // Read the RAW samples from the microphone
        i2s_read((i2s_port_t)EXAMPLE_I2S_CH, (char *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 100);
        // Write the samples to the WAV file
        file.write((uint8_t *)i2s_readraw_buff, bytes_read);
        flash_wr_size += bytes_read;
    }
    Serial.println("Recording done!");
    file.close();
    Serial.println("File written on SDCard");

    mic_recode_stop();
}



uint32_t flash_rec_time;
File recode_file;
void mic_recode_start(uint32_t rec_time)
{
    recode_start_flag = true;
    // char wav_header_fmt[WAVE_HEADER_SIZE];
    // flash_rec_time = BYTE_RATE * rec_time;
    // generate_wav_header(wav_header_fmt, flash_rec_time, EXAMPLE_SAMPLE_RATE);

    // // Create new WAV file
    // char buf[32]={0};
    // snprintf(buf, 32, "/record%02d.wav", recode_cnt);
    // recode_cnt++;
    // Serial.printf("file: %s\n", buf);

    // File file = SD.open(buf, FILE_WRITE);
    // if(!file){
    //     Serial.println("Failed to open file for writing");
    //     return;
    // } else {
    //     Serial.printf("Open file: %s\n", buf);
    // }
    
    // file.write((uint8_t *)wav_header_fmt, WAVE_HEADER_SIZE);

}

void mic_task(void *param)
{
    int flash_wr_size = 0;

    while(1) {
        taskENTER_CRITICAL(&mic_spinlock);
        // record 
        if(recode_start_flag) {
            if(flash_wr_size < flash_rec_time) {
                // Read the RAW samples from the microphone
                i2s_read((i2s_port_t)EXAMPLE_I2S_CH, (char *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 100);
                // Write the samples to the WAV file
                recode_file.write((uint8_t *)i2s_readraw_buff, bytes_read);
                flash_wr_size += bytes_read;
            } else {
                mic_recode_stop();
            }
        }
        taskEXIT_CRITICAL(&mic_spinlock);
        delay(1);
    }
}

void mic_recode_stop(void)
{
    recode_start_flag = false;
    // Serial.println("Recording done!");
    // recode_file.close();
    // Serial.println("File written on SDCard");
}

bool mic_recode_st(void)
{
    return recode_start_flag;
}

void mic_init(void)
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

