
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <sys/stat.h>
#include "driver/i2s.h"
#include "utilities.h"

#define EXAMPLE_REC_TIME    15       // Recording time
#define EXAMPLE_I2S_CH      0        // I2S Channel Number
#define EXAMPLE_SAMPLE_RATE 44100    // Audio Sample Rate  44.1KHz
#define EXAMPLE_BIT_SAMPLE  16       // Audio Bit Sample

#define SPI_DMA_CHAN        SPI_DMA_CH_AUTO
#define NUM_CHANNELS        (1) // For mono recording only!
#define SD_MOUNT_POINT      "/sdcard"
#define SAMPLE_SIZE         (EXAMPLE_BIT_SAMPLE * 1024)
#define BYTE_RATE           (EXAMPLE_SAMPLE_RATE * (EXAMPLE_BIT_SAMPLE / 8)) * NUM_CHANNELS

static int16_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;

#define REASSIGN_PINS
int sck =  BOARD_SD_SCK;
int miso = BOARD_SPI_MISO;
int mosi = BOARD_SPI_MOSI;
int cs =   BOARD_SD_CS;


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
    File file = SD.open("/record.wav", FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
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
}

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

void setup(){
    Serial.begin(115200);
    while(!Serial) { delay (10); }

#ifdef REASSIGN_PINS
    SPI.begin(sck, miso, mosi);
#endif
    //if(!SD.begin(cs)){ //Change to this function to manually change CS pin
    if(!SD.begin(cs)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    // Init the PDM digital microphone
    init_microphone();
    Serial.printf("Starting recording for %d seconds!\n", EXAMPLE_REC_TIME);
    // Start Recording
    record_wav(EXAMPLE_REC_TIME); 
    // Stop I2S driver and destroy
    ESP_ERROR_CHECK( i2s_driver_uninstall((i2s_port_t)EXAMPLE_I2S_CH) );
}

void loop(){
    delay(100000);
}
