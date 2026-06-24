#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <RotaryEncoder.h>
#include <driver/i2s.h>

#include "Audio.h"
#include "utilities.h"

namespace {

constexpr uint8_t kRotation = 3;
constexpr uint32_t kBusSettleMs = 20;
constexpr uint32_t kSdPowerSettleMs = 120;
constexpr uint32_t kSdRetryIntervalMs = 800;
constexpr uint32_t kUiFrameIntervalMs = 140;
constexpr uint32_t kButtonDebounceMs = 30;

constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr i2s_port_t kSpeakerPort = I2S_NUM_1;
constexpr uint32_t kSampleRate = 16000;
constexpr uint8_t kBitsPerSample = 16;
constexpr uint8_t kNumChannels = 1;
constexpr uint32_t kRecordDurationSec = 10;
constexpr size_t kRecordChunkSamples = 512;
constexpr size_t kRecordChunkBytes = kRecordChunkSamples * sizeof(int16_t);
constexpr size_t kWavHeaderSize = 44;
constexpr uint32_t kRecordDataBytes =
    kSampleRate * (kBitsPerSample / 8U) * kNumChannels * kRecordDurationSec;

constexpr uint32_t kSdMountFrequencies[] = {10000000UL, 4000000UL, 1000000UL};
constexpr char kRecordPath[] = "/record.wav";
constexpr uint8_t kAudioVolume = 21;
constexpr uint8_t kRecordGain = 2;

constexpr uint16_t kColorBg = 0x0841;
constexpr uint16_t kColorPanel = 0x1082;
constexpr uint16_t kColorPanelEdge = 0x31A6;
constexpr uint16_t kColorCard = 0x18C3;
constexpr uint16_t kColorPassBg = 0x0A41;
constexpr uint16_t kColorFailBg = 0x3006;
constexpr uint16_t kColorActiveBg = 0x1247;

enum class State : uint8_t {
    WAIT_SD = 0,
    RECORDING,
    READY,
    PLAYING,
    ERROR,
};

enum class CardState : uint8_t {
    WAIT = 0,
    ACTIVE,
    PASS,
    FAIL,
};

enum class FailureOrigin : uint8_t {
    NONE = 0,
    RECORD,
    PLAYBACK,
};

enum class RecordResult : uint8_t {
    SUCCESS = 0,
    RESTART,
    NO_SD,
    FAILURE,
};

struct DebouncedButton {
    explicit DebouncedButton(uint8_t pinNumber) : pin(pinNumber) {}

    void begin()
    {
        pinMode(pin, INPUT_PULLUP);
        rawState = digitalRead(pin);
        stableState = rawState;
        lastChangeMs = millis();
    }

    bool pressed()
    {
        const bool currentRaw = digitalRead(pin);
        const uint32_t now = millis();

        if (currentRaw != rawState) {
            rawState = currentRaw;
            lastChangeMs = now;
        }

        if ((now - lastChangeMs) >= kButtonDebounceMs && stableState != rawState) {
            stableState = rawState;
            if (stableState == LOW) {
                return true;
            }
        }

        return false;
    }

    uint8_t pin;
    bool rawState = HIGH;
    bool stableState = HIGH;
    uint32_t lastChangeMs = 0;
};

TFT_eSPI tft;
TFT_eSprite canvas(&tft);
RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);
Audio audio(false, 3, kSpeakerPort);
DebouncedButton userButton(BOARD_USER_KEY);
DebouncedButton encoderButton(ENCODER_KEY);

State gState = State::WAIT_SD;
FailureOrigin gFailureOrigin = FailureOrigin::NONE;
uint32_t gStateEnterMs = 0;
uint32_t gLastUiDrawMs = 0;
uint32_t gLastSdProbeMs = 0;
bool gScreenDirty = true;
bool gCanvasReady = false;
bool gSdMounted = false;
bool gHasRecording = false;
uint32_t gMountedFrequency = 0;
uint64_t gCardTotalMB = 0;
uint64_t gCardUsedMB = 0;
uint32_t gRecordBytesWritten = 0;
uint32_t gRecordedFileSize = 0;
uint16_t gRecordRms = 0;
uint16_t gRecordPeak = 0;
uint8_t gMenuIndex = 0;
long gLastEncoderPosition = 0;
char gErrorTitle[48] = {0};
char gErrorDetail[96] = {0};

void redrawScreen();

void deselectSharedSpiDevices()
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

SPIClass &sharedSpi()
{
    return tft.getSPIinstance();
}

void markDirty()
{
    gScreenDirty = true;
}

void resetEncoderSelection(uint8_t index)
{
    gMenuIndex = index % 2U;
    encoder.setPosition(gMenuIndex);
    gLastEncoderPosition = gMenuIndex;
    markDirty();
}

void clearFailure()
{
    gFailureOrigin = FailureOrigin::NONE;
    gErrorTitle[0] = '\0';
    gErrorDetail[0] = '\0';
}

void setState(State state)
{
    gState = state;
    gStateEnterMs = millis();
    markDirty();
}

void setError(FailureOrigin origin, const char *title, const char *detail)
{
    gFailureOrigin = origin;
    strncpy(gErrorTitle, title, sizeof(gErrorTitle) - 1U);
    gErrorTitle[sizeof(gErrorTitle) - 1U] = '\0';
    strncpy(gErrorDetail, detail, sizeof(gErrorDetail) - 1U);
    gErrorDetail[sizeof(gErrorDetail) - 1U] = '\0';
    setState(State::ERROR);
}

bool initDisplayPower()
{
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);

    deselectSharedSpiDevices();
    delay(kBusSettleMs);

    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, LOW);

    tft.begin();
    tft.setRotation(kRotation);
    tft.fillScreen(TFT_BLACK);

    digitalWrite(DISPLAY_BL, HIGH);
    return true;
}

bool mountSdCard(uint32_t &mountedFrequency)
{
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    delay(kSdPowerSettleMs);

    for (const uint32_t frequency : kSdMountFrequencies) {
        SD.end();
        deselectSharedSpiDevices();
        delay(kBusSettleMs);

        if (!SD.begin(BOARD_SD_CS, sharedSpi(), frequency)) {
            continue;
        }

        if (SD.cardType() == CARD_NONE) {
            SD.end();
            continue;
        }

        mountedFrequency = frequency;
        return true;
    }

    mountedFrequency = 0;
    return false;
}

void clearSdInfo()
{
    gSdMounted = false;
    gMountedFrequency = 0;
    gCardTotalMB = 0;
    gCardUsedMB = 0;
}

bool sdAccessible()
{
    if (!gSdMounted) {
        return false;
    }

    File root = SD.open("/");
    if (!root) {
        return false;
    }

    root.close();
    return true;
}

void refreshSdStats()
{
    if (!gSdMounted) {
        return;
    }

    gCardTotalMB = SD.totalBytes() / (1024ULL * 1024ULL);
    gCardUsedMB = SD.usedBytes() / (1024ULL * 1024ULL);
}

bool ensureSdMounted()
{
    if (sdAccessible()) {
        return true;
    }

    clearSdInfo();

    uint32_t mountedFrequency = 0;
    if (!mountSdCard(mountedFrequency)) {
        return false;
    }

    gSdMounted = true;
    gMountedFrequency = mountedFrequency;
    refreshSdStats();
    return true;
}

String formatBytes(uint32_t bytes)
{
    if (bytes >= 1024UL * 1024UL) {
        return String(static_cast<float>(bytes) / (1024.0f * 1024.0f), 1) + "M";
    }
    if (bytes >= 1024UL) {
        return String((bytes + 512UL) / 1024UL) + "K";
    }
    return String(bytes) + "B";
}

uint32_t recordElapsedMs()
{
    return (gRecordBytesWritten * 1000UL) /
           (kSampleRate * kNumChannels * (kBitsPerSample / 8U));
}

uint16_t recordProgressPercent()
{
    if (kRecordDataBytes == 0) {
        return 0;
    }

    const uint32_t progress = (gRecordBytesWritten * 100UL) / kRecordDataBytes;
    return progress > 100U ? 100U : static_cast<uint16_t>(progress);
}

uint16_t computeRms(const int16_t *samples, size_t count)
{
    if (count == 0) {
        return 0;
    }

    int64_t sum = 0;
    for (size_t index = 0; index < count; ++index) {
        const int32_t sample = samples[index];
        sum += sample * sample;
    }

    return static_cast<uint16_t>(sqrt(static_cast<double>(sum) / count));
}

int16_t applySampleGain(int16_t sample)
{
    int32_t scaled = static_cast<int32_t>(sample) * kRecordGain;
    if (scaled > INT16_MAX) {
        scaled = INT16_MAX;
    } else if (scaled < INT16_MIN) {
        scaled = INT16_MIN;
    }
    return static_cast<int16_t>(scaled);
}

void generateWavHeader(uint8_t *wavHeader, uint32_t wavSize)
{
    const uint32_t fileSize = wavSize + kWavHeaderSize - 8U;
    const uint32_t byteRate = kSampleRate * (kBitsPerSample / 8U) * kNumChannels;
    const uint16_t blockAlign = (kBitsPerSample / 8U) * kNumChannels;

    const uint8_t header[kWavHeaderSize] = {
        'R', 'I', 'F', 'F',
        static_cast<uint8_t>(fileSize & 0xFF),
        static_cast<uint8_t>((fileSize >> 8) & 0xFF),
        static_cast<uint8_t>((fileSize >> 16) & 0xFF),
        static_cast<uint8_t>((fileSize >> 24) & 0xFF),
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        0x10, 0x00, 0x00, 0x00,
        0x01, 0x00,
        static_cast<uint8_t>(kNumChannels), 0x00,
        static_cast<uint8_t>(kSampleRate & 0xFF),
        static_cast<uint8_t>((kSampleRate >> 8) & 0xFF),
        static_cast<uint8_t>((kSampleRate >> 16) & 0xFF),
        static_cast<uint8_t>((kSampleRate >> 24) & 0xFF),
        static_cast<uint8_t>(byteRate & 0xFF),
        static_cast<uint8_t>((byteRate >> 8) & 0xFF),
        static_cast<uint8_t>((byteRate >> 16) & 0xFF),
        static_cast<uint8_t>((byteRate >> 24) & 0xFF),
        static_cast<uint8_t>(blockAlign & 0xFF),
        static_cast<uint8_t>((blockAlign >> 8) & 0xFF),
        static_cast<uint8_t>(kBitsPerSample), 0x00,
        'd', 'a', 't', 'a',
        static_cast<uint8_t>(wavSize & 0xFF),
        static_cast<uint8_t>((wavSize >> 8) & 0xFF),
        static_cast<uint8_t>((wavSize >> 16) & 0xFF),
        static_cast<uint8_t>((wavSize >> 24) & 0xFF),
    };

    memcpy(wavHeader, header, sizeof(header));
}

bool initMic()
{
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    config.sample_rate = kSampleRate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
    config.dma_buf_count = 8;
    config.dma_buf_len = 200;
    config.use_apll = false;
    config.tx_desc_auto_clear = false;
    config.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = I2S_PIN_NO_CHANGE;
    pins.ws_io_num = BOARD_MIC_CLK;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num = BOARD_MIC_DATA;

    i2s_driver_uninstall(kMicPort);

    if (i2s_driver_install(kMicPort, &config, 0, nullptr) != ESP_OK) {
        return false;
    }
    if (i2s_set_pin(kMicPort, &pins) != ESP_OK) {
        i2s_driver_uninstall(kMicPort);
        return false;
    }
    if (i2s_set_clk(kMicPort, kSampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO) != ESP_OK) {
        i2s_driver_uninstall(kMicPort);
        return false;
    }

    i2s_zero_dma_buffer(kMicPort);
    return true;
}

void deinitMic()
{
    i2s_driver_uninstall(kMicPort);
}

void stopPlayback(bool backToReady)
{
    if (audio.isRunning()) {
        audio.stopSong();
        delay(kBusSettleMs);
    }

    if (backToReady && gSdMounted) {
        setState(State::READY);
        resetEncoderSelection(0);
    }
}

bool startPlayback()
{
    if (!gHasRecording) {
        setError(FailureOrigin::PLAYBACK, "No recording available",
                 "Please record a new 10 second clip first.");
        return false;
    }

    if (!ensureSdMounted()) {
        setState(State::WAIT_SD);
        return false;
    }

    stopPlayback(false);

    if (!SD.exists(kRecordPath)) {
        gHasRecording = false;
        gRecordedFileSize = 0;
        setError(FailureOrigin::PLAYBACK, "record.wav missing",
                 "Insert SD card and record a new sample.");
        return false;
    }

    if (!audio.connecttoFS(SD, kRecordPath)) {
        setError(FailureOrigin::PLAYBACK, "Playback start failed",
                 "Audio could not open the WAV file from SD.");
        return false;
    }

    clearFailure();
    setState(State::PLAYING);
    resetEncoderSelection(0);
    Serial.println(F("[PLAY] Playing /record.wav"));
    return true;
}

RecordResult recordOnce()
{
    if (!ensureSdMounted()) {
        return RecordResult::NO_SD;
    }

    if (!initMic()) {
        setError(FailureOrigin::RECORD, "Mic init failed",
                 "Unable to start the PDM microphone path.");
        return RecordResult::FAILURE;
    }

    if (SD.exists(kRecordPath)) {
        SD.remove(kRecordPath);
    }

    File file = SD.open(kRecordPath, FILE_WRITE);
    if (!file) {
        deinitMic();
        setError(FailureOrigin::RECORD, "File open failed",
                 "Could not create /record.wav on the SD card.");
        return RecordResult::FAILURE;
    }

    uint8_t wavHeader[kWavHeaderSize];
    generateWavHeader(wavHeader, kRecordDataBytes);
    if (file.write(wavHeader, kWavHeaderSize) != kWavHeaderSize) {
        file.close();
        SD.remove(kRecordPath);
        deinitMic();
        setError(FailureOrigin::RECORD, "Header write failed",
                 "The WAV file header could not be saved.");
        return RecordResult::FAILURE;
    }

    clearFailure();
    gRecordBytesWritten = 0;
    gRecordRms = 0;
    gRecordPeak = 0;
    setState(State::RECORDING);
    redrawScreen();
    gScreenDirty = false;
    gLastUiDrawMs = millis();

    uint32_t lastUiRefreshMs = gLastUiDrawMs;
    int16_t sampleBuffer[kRecordChunkSamples];

    while (gRecordBytesWritten < kRecordDataBytes) {
        size_t bytesRead = 0;
        const esp_err_t readResult = i2s_read(
            kMicPort, sampleBuffer, kRecordChunkBytes, &bytesRead, 100);

        if (readResult != ESP_OK || bytesRead == 0) {
            continue;
        }

        const size_t bytesToWrite = min(bytesRead, static_cast<size_t>(kRecordDataBytes - gRecordBytesWritten));
        const size_t sampleCount = bytesToWrite / sizeof(int16_t);

        for (size_t index = 0; index < sampleCount; ++index) {
            sampleBuffer[index] = applySampleGain(sampleBuffer[index]);
        }

        const size_t bytesWritten = file.write(reinterpret_cast<const uint8_t *>(sampleBuffer), bytesToWrite);
        if (bytesWritten != bytesToWrite) {
            file.close();
            SD.remove(kRecordPath);
            deinitMic();
            setError(FailureOrigin::RECORD, "Write failed",
                     "Audio data could not be fully written to SD.");
            return RecordResult::FAILURE;
        }

        gRecordBytesWritten += bytesWritten;

        if (sampleCount > 0) {
            gRecordRms = computeRms(sampleBuffer, sampleCount);
            if (gRecordRms > gRecordPeak) {
                gRecordPeak = gRecordRms;
            } else {
                gRecordPeak = (gRecordPeak * 31U) / 32U;
            }
        }

        if (userButton.pressed()) {
            file.close();
            SD.remove(kRecordPath);
            deinitMic();
            gRecordBytesWritten = 0;
            gRecordRms = 0;
            gRecordPeak = 0;
            Serial.println(F("[REC] USER key pressed, restarting take."));
            return RecordResult::RESTART;
        }

        const uint32_t now = millis();
        if (now - lastUiRefreshMs >= kUiFrameIntervalMs) {
            markDirty();
            redrawScreen();
            gScreenDirty = false;
            gLastUiDrawMs = now;
            lastUiRefreshMs = now;
        }
    }

    file.close();
    deinitMic();
    refreshSdStats();

    File verify = SD.open(kRecordPath, FILE_READ);
    if (verify) {
        gRecordedFileSize = verify.size();
        verify.close();
    } else {
        gRecordedFileSize = gRecordBytesWritten + kWavHeaderSize;
    }

    gHasRecording = true;
    clearFailure();
    setState(State::READY);
    resetEncoderSelection(0);
    Serial.printf("[REC] Saved %lu bytes to %s\n", gRecordedFileSize, kRecordPath);
    return RecordResult::SUCCESS;
}

void startRecordingWorkflow()
{
    stopPlayback(false);

    while (true) {
        if (!ensureSdMounted()) {
            clearSdInfo();
            setState(State::WAIT_SD);
            return;
        }

        const RecordResult result = recordOnce();
        if (result == RecordResult::RESTART) {
            delay(80);
            continue;
        }
        if (result == RecordResult::NO_SD) {
            clearSdInfo();
            setState(State::WAIT_SD);
            return;
        }
        return;
    }
}

CardState sdCardState()
{
    return gSdMounted ? CardState::PASS : CardState::WAIT;
}

CardState recordCardState()
{
    if (gState == State::RECORDING) {
        return CardState::ACTIVE;
    }
    if (!gSdMounted) {
        return CardState::WAIT;
    }
    if (gFailureOrigin == FailureOrigin::RECORD) {
        return CardState::FAIL;
    }
    return gHasRecording ? CardState::PASS : CardState::WAIT;
}

CardState playCardState()
{
    if (gState == State::PLAYING) {
        return CardState::ACTIVE;
    }
    if (!gSdMounted) {
        return CardState::WAIT;
    }
    if (gFailureOrigin == FailureOrigin::PLAYBACK) {
        return CardState::FAIL;
    }
    return gHasRecording ? CardState::PASS : CardState::WAIT;
}

#include "test_record_audio_ui.h"

void redrawScreen()
{
    deselectSharedSpiDevices();

    if (gCanvasReady) {
        drawUi(canvas);
        canvas.pushSprite(0, 0);
    } else {
        drawUi(tft);
    }

    deselectSharedSpiDevices();
}

void updateEncoderSelection()
{
    if (gState != State::READY && gState != State::PLAYING && gState != State::ERROR) {
        return;
    }

    const long position = encoder.getPosition();
    if (position == gLastEncoderPosition) {
        return;
    }

    gLastEncoderPosition = position;
    long normalized = position % 2L;
    if (normalized < 0) {
        normalized += 2L;
    }

    const uint8_t nextIndex = static_cast<uint8_t>(normalized);
    if (nextIndex != gMenuIndex) {
        gMenuIndex = nextIndex;
        markDirty();
    }
}

void handleEncoderConfirm()
{
    switch (gState) {
    case State::READY:
        if (gMenuIndex == 0U) {
            startPlayback();
        } else {
            startRecordingWorkflow();
        }
        break;

    case State::PLAYING:
        if (gMenuIndex == 0U) {
            stopPlayback(true);
        } else {
            startRecordingWorkflow();
        }
        break;

    case State::ERROR:
        startRecordingWorkflow();
        break;

    case State::WAIT_SD:
    case State::RECORDING:
    default:
        break;
    }
}

void pollInputs()
{
    encoder.tick();
    updateEncoderSelection();

    if (gState != State::RECORDING && userButton.pressed()) {
        Serial.println(F("[REC] USER key pressed, starting a new take."));
        startRecordingWorkflow();
        return;
    }

    if (gState != State::RECORDING && encoderButton.pressed()) {
        handleEncoderConfirm();
    }
}

void updateStateMachine()
{
    if (gState == State::WAIT_SD) {
        const uint32_t now = millis();
        if (now - gLastSdProbeMs >= kSdRetryIntervalMs) {
            gLastSdProbeMs = now;
            if (ensureSdMounted()) {
                Serial.println(F("[SD] Card detected, starting recording."));
                startRecordingWorkflow();
            }
        }
        return;
    }

    if (gState == State::PLAYING) {
        if (audio.isRunning()) {
            audio.loop();
        } else {
            refreshSdStats();
            setState(State::READY);
            resetEncoderSelection(0);
            Serial.println(F("[PLAY] Playback finished."));
        }
    }
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("T-Embed Record Audio Test"));

    if (!initDisplayPower()) {
        Serial.println(F("[REC] Display power init failed - halting."));
        while (true) {
            delay(1000);
        }
    }

    userButton.begin();
    encoderButton.begin();

    canvas.setColorDepth(16);
    gCanvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!gCanvasReady) {
        Serial.println(F("[REC] Sprite allocation failed, using direct TFT redraw."));
    }

    if (!audio.setPinout(BOARD_VOICE_BCLK, BOARD_VOICE_LRCLK, BOARD_VOICE_DIN)) {
        Serial.println(F("[PLAY] Speaker pinout init failed."));
    }
    audio.setVolume(kAudioVolume);

    resetEncoderSelection(0);
    setState(State::WAIT_SD);
    redrawScreen();
    gScreenDirty = false;
    gLastUiDrawMs = millis();
}

void loop()
{
    pollInputs();
    updateStateMachine();

    if (gScreenDirty &&
        (gLastUiDrawMs == 0 || millis() - gLastUiDrawMs >= kUiFrameIntervalMs)) {
        redrawScreen();
        gScreenDirty = false;
        gLastUiDrawMs = millis();
    }

    delay(gState == State::PLAYING ? 2 : 10);
}

void audio_info(const char *info)
{
    Serial.print(F("[AUDIO] "));
    Serial.println(info);
}
