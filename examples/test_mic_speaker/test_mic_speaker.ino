#include <Arduino.h>
#include <TFT_eSPI.h>
#include <driver/i2s.h>
#include <math.h>

#include "utilities.h"

namespace {

constexpr uint8_t kRotation = 3;
constexpr int16_t kUiMargin = 8;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;
constexpr uint32_t kUiFrameIntervalMs = 33;
constexpr uint32_t kBusSettleMs = 20;

constexpr int16_t kStageCardY = 32;
constexpr int16_t kStageCardW = 96;
constexpr int16_t kStageCardH = 28;
constexpr int16_t kStageGap = 8;

constexpr int16_t kPanelX = 8;
constexpr int16_t kPanelY = 68;
constexpr int16_t kPanelW = 304;
constexpr int16_t kPanelH = 52;

constexpr int16_t kMetricY = 128;
constexpr int16_t kMetricW = 96;
constexpr int16_t kMetricH = 20;

constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr i2s_port_t kSpkPort = I2S_NUM_1;

constexpr int kSampleRate = 16000;
constexpr size_t kBufSamples = 512;
constexpr size_t kSpeakerChunkSamples = 1024;
constexpr uint8_t kSpeakerWarmupChunks = 2;

constexpr uint16_t kMicSignalThreshold = 200;
constexpr uint32_t kMicTestDurationMs = 5000;
constexpr uint32_t kSpkFreqDwellMs = 1500;
constexpr uint32_t kLoopbackDurationMs = 8000;

constexpr int16_t kToneAmplitude = 12000;
constexpr float kToneFreqs[] = {440.0f, 1000.0f, 880.0f};
constexpr float kTwoPi = 6.28318530718f;

constexpr size_t kLoopBufSamples = 8192;

constexpr uint16_t kColorBg = 0x0841;
constexpr uint16_t kColorPanel = 0x1082;
constexpr uint16_t kColorPanelEdge = 0x31A6;
constexpr uint16_t kColorCard = 0x18C3;
constexpr uint16_t kColorMeterBg = 0x2104;
constexpr uint16_t kColorPassBg = 0x0A41;
constexpr uint16_t kColorFailBg = 0x3006;

enum class State : uint8_t {
    INIT_MIC = 0,
    MIC_TEST,
    INIT_SPK,
    SPEAKER_TEST,
    INIT_LOOPBACK,
    LOOPBACK_TEST,
    DONE_PASS,
    DONE_FAIL,
};

TFT_eSPI tft;
TFT_eSprite canvas(&tft);

State gState = State::INIT_MIC;
uint32_t gStateEnterMs = 0;
uint32_t gLastUiDrawMs = 0;
bool gScreenDirty = true;
bool gCanvasReady = false;

bool gMicDriverOk = false;
bool gSpkDriverOk = false;
bool gMicOk = false;
bool gSpkOk = false;
bool gLoopOk = false;
bool gMicStarted = false;
bool gSpkStarted = false;
bool gLoopStarted = false;

uint16_t gVuRms = 0;
uint16_t gVuPeak = 0;
uint32_t gVuPeakDecayMs = 0;

uint8_t gFreqIndex = 0;
uint16_t gTonePhase = 0;
uint32_t gFreqLastChangeMs = 0;

int16_t gLoopBuf[kLoopBufSamples];
size_t gLoopWrite = 0;
size_t gLoopRead = 0;
bool gLoopFilled = false;

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

void markDirty()
{
    gScreenDirty = true;
}

void setState(State state)
{
    gState = state;
    gStateEnterMs = millis();
    gLastUiDrawMs = 0;
    markDirty();
}

uint16_t computeRms(const int16_t *buffer, size_t count)
{
    if (count == 0) {
        return 0;
    }

    int64_t sum = 0;
    for (size_t index = 0; index < count; ++index) {
        const int32_t sample = buffer[index];
        sum += sample * sample;
    }
    return static_cast<uint16_t>(sqrt(static_cast<double>(sum) / count));
}

void fillTone(int16_t *out, size_t count, float freq, uint16_t &phase)
{
    const uint16_t increment = static_cast<uint16_t>(freq * 65536.0f / kSampleRate);
    for (size_t index = 0; index < count; ++index) {
        out[index] = static_cast<int16_t>(kToneAmplitude * sinf(phase * (kTwoPi / 65536.0f)));
        phase += increment;
    }
}

void writeSpeakerSamples(const int16_t *samples, size_t count)
{
    if (!gSpkDriverOk || count == 0) {
        return;
    }

    const uint8_t *data = reinterpret_cast<const uint8_t *>(samples);
    size_t remaining = count * sizeof(int16_t);
    while (remaining > 0) {
        size_t bytesWritten = 0;
        i2s_write(kSpkPort, data, remaining, &bytesWritten, portMAX_DELAY);
        if (bytesWritten == 0) {
            break;
        }
        data += bytesWritten;
        remaining -= bytesWritten;
    }
}

void writeToneChunk()
{
    int16_t buffer[kSpeakerChunkSamples];
    fillTone(buffer, kSpeakerChunkSamples, kToneFreqs[gFreqIndex], gTonePhase);
    writeSpeakerSamples(buffer, kSpeakerChunkSamples);
}

bool initMic()
{
    if (gMicDriverOk) {
        return true;
    }

    i2s_config_t cfg = {};
    cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    cfg.sample_rate = kSampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 4;
    cfg.dma_buf_len = 256;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = false;
    cfg.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = I2S_PIN_NO_CHANGE;
    pins.ws_io_num = BOARD_MIC_CLK;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num = BOARD_MIC_DATA;

    if (i2s_driver_install(kMicPort, &cfg, 0, nullptr) != ESP_OK) {
        return false;
    }
    if (i2s_set_pin(kMicPort, &pins) != ESP_OK) {
        i2s_driver_uninstall(kMicPort);
        return false;
    }

    i2s_zero_dma_buffer(kMicPort);
    gMicDriverOk = true;
    return true;
}

void deinitMic()
{
    if (!gMicDriverOk) {
        return;
    }

    i2s_driver_uninstall(kMicPort);
    gMicDriverOk = false;
}

bool initSpeaker()
{
    if (gSpkDriverOk) {
        return true;
    }

    i2s_config_t cfg = {};
    cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate = kSampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 8;
    cfg.dma_buf_len = 256;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = true;
    cfg.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = BOARD_VOICE_BCLK;
    pins.ws_io_num = BOARD_VOICE_LRCLK;
    pins.data_out_num = BOARD_VOICE_DIN;
    pins.data_in_num = I2S_PIN_NO_CHANGE;

    if (i2s_driver_install(kSpkPort, &cfg, 0, nullptr) != ESP_OK) {
        return false;
    }
    if (i2s_set_pin(kSpkPort, &pins) != ESP_OK) {
        i2s_driver_uninstall(kSpkPort);
        return false;
    }

    i2s_zero_dma_buffer(kSpkPort);
    gSpkDriverOk = true;
    return true;
}

void deinitSpeaker()
{
    if (!gSpkDriverOk) {
        return;
    }

    i2s_zero_dma_buffer(kSpkPort);
    i2s_driver_uninstall(kSpkPort);
    gSpkDriverOk = false;
}

void resetTestState()
{
    gMicDriverOk = false;
    gSpkDriverOk = false;
    gMicOk = false;
    gSpkOk = false;
    gLoopOk = false;
    gMicStarted = false;
    gSpkStarted = false;
    gLoopStarted = false;
    gVuRms = 0;
    gVuPeak = 0;
    gVuPeakDecayMs = 0;
    gFreqIndex = 0;
    gTonePhase = 0;
    gLoopWrite = 0;
    gLoopRead = 0;
    gLoopFilled = false;
    setState(State::INIT_MIC);
}

#include "test_mic_speaker_ui.h"

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

void updateStateMachine()
{
    const uint32_t now = millis();

    switch (gState) {
    case State::INIT_MIC:
        gMicStarted = true;
        gVuRms = 0;
        gVuPeak = 0;
        gVuPeakDecayMs = now;
        Serial.println(F("[AUD] Starting mic test (5 s - make some noise)..."));
        if (!initMic()) {
            Serial.println(F("[AUD] Mic I2S init failed."));
            setState(State::DONE_FAIL);
        } else {
            setState(State::MIC_TEST);
        }
        break;

    case State::MIC_TEST: {
        int16_t buffer[kBufSamples];
        size_t bytesRead = 0;
        i2s_read(kMicPort, buffer, sizeof(buffer), &bytesRead, 0);
        const size_t sampleCount = bytesRead / sizeof(int16_t);
        if (sampleCount > 0) {
            const uint16_t rms = computeRms(buffer, sampleCount);
            gVuRms = rms;
            if (rms > gVuPeak) {
                gVuPeak = rms;
                gVuPeakDecayMs = now;
            }
            if (now - gVuPeakDecayMs > 800) {
                gVuPeak = (gVuPeak * 15) / 16;
            }
            if (rms > kMicSignalThreshold) {
                gMicOk = true;
            }
            markDirty();
        }
        if (now - gStateEnterMs >= kMicTestDurationMs) {
            deinitMic();
            Serial.print(F("[AUD] Mic test "));
            Serial.println(gMicOk ? F("PASS") : F("FAIL (no signal)"));
            setState(State::INIT_SPK);
        }
        break;
    }

    case State::INIT_SPK:
        gSpkStarted = true;
        gVuRms = 0;
        gVuPeak = 0;
        gVuPeakDecayMs = now;
        gFreqIndex = 0;
        gTonePhase = 0;
        gFreqLastChangeMs = now;
        Serial.println(F("[AUD] Starting speaker test (3 tones)..."));
        if (!initSpeaker()) {
            Serial.println(F("[AUD] Speaker I2S init failed."));
            gSpkOk = false;
            setState(State::INIT_LOOPBACK);
        } else {
            gSpkOk = true;
            for (uint8_t index = 0; index < kSpeakerWarmupChunks; ++index) {
                writeToneChunk();
            }
            setState(State::SPEAKER_TEST);
        }
        break;

    case State::SPEAKER_TEST:
        writeToneChunk();

        if (now - gFreqLastChangeMs >= kSpkFreqDwellMs) {
            gFreqIndex = (gFreqIndex + 1U) % 3U;
            gFreqLastChangeMs = now;
            markDirty();
            Serial.print(F("[AUD] Tone -> "));
            Serial.print(kToneFreqs[gFreqIndex]);
            Serial.println(F(" Hz"));
        }

        if (now - gStateEnterMs >= kSpkFreqDwellMs * 3U) {
            deinitSpeaker();
            Serial.println(F("[AUD] Speaker test done."));
            setState(State::INIT_LOOPBACK);
        }
        break;

    case State::INIT_LOOPBACK:
        gLoopStarted = true;
        gLoopWrite = 0;
        gLoopRead = 0;
        gLoopFilled = false;
        gVuRms = 0;
        gVuPeak = 0;
        gVuPeakDecayMs = now;
        Serial.println(F("[AUD] Starting loopback test (8 s - speak into mic)..."));
        if (!initMic()) {
            Serial.println(F("[AUD] Loopback mic init failed."));
            setState(State::DONE_FAIL);
            break;
        }
        if (!initSpeaker()) {
            Serial.println(F("[AUD] Loopback speaker init failed."));
            deinitMic();
            setState(State::DONE_FAIL);
            break;
        }
        setState(State::LOOPBACK_TEST);
        break;

    case State::LOOPBACK_TEST: {
        int16_t inputBuffer[kBufSamples];
        size_t bytesRead = 0;
        i2s_read(kMicPort, inputBuffer, sizeof(inputBuffer), &bytesRead, 0);
        const size_t sampleCount = bytesRead / sizeof(int16_t);

        for (size_t index = 0; index < sampleCount; ++index) {
            gLoopBuf[gLoopWrite] = inputBuffer[index];
            gLoopWrite = (gLoopWrite + 1U) % kLoopBufSamples;
            if (gLoopWrite == 0U) {
                gLoopFilled = true;
            }
        }

        if (sampleCount > 0) {
            gVuRms = computeRms(inputBuffer, sampleCount);
            if (gVuRms > gVuPeak) {
                gVuPeak = gVuRms;
                gVuPeakDecayMs = now;
            }
            if (now - gVuPeakDecayMs > 800) {
                gVuPeak = (gVuPeak * 15) / 16;
            }
            if (gVuRms > kMicSignalThreshold) {
                gLoopOk = true;
            }
            markDirty();
        }

        if (gLoopFilled) {
            int16_t outputBuffer[kBufSamples];
            for (size_t index = 0; index < kBufSamples; ++index) {
                outputBuffer[index] = gLoopBuf[gLoopRead];
                gLoopRead = (gLoopRead + 1U) % kLoopBufSamples;
            }
            writeSpeakerSamples(outputBuffer, kBufSamples);
        }

        if (now - gStateEnterMs >= kLoopbackDurationMs) {
            deinitMic();
            deinitSpeaker();
            Serial.print(F("[AUD] Loopback test "));
            Serial.println(gLoopOk ? F("PASS") : F("FAIL (no mic signal)"));
            const bool allPass = gMicOk && gSpkOk && gLoopOk;
            setState(allPass ? State::DONE_PASS : State::DONE_FAIL);
        }
        break;
    }

    case State::DONE_PASS:
    case State::DONE_FAIL:
        break;
    }
}

void pollUserKey()
{
    static bool lastPressed = false;

    const bool pressed = (digitalRead(BOARD_USER_KEY) == LOW);
    if (pressed && !lastPressed) {
        if (gState == State::DONE_PASS || gState == State::DONE_FAIL) {
            Serial.println(F("[AUD] Restarting test..."));
            resetTestState();
        }
    }

    lastPressed = pressed;
}

} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println(F("T-Embed Mic & Speaker Test"));

    pinMode(BOARD_USER_KEY, INPUT_PULLUP);

    if (!initDisplayPower()) {
        Serial.println(F("[AUD] Board init failed - halting."));
        while (true) {
            delay(1000);
        }
    }

    canvas.setColorDepth(16);
    gCanvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!gCanvasReady) {
        Serial.println(F("[AUD] Sprite allocation failed, using direct TFT redraw."));
    }

    resetTestState();
    redrawScreen();
    gScreenDirty = false;
    gLastUiDrawMs = millis();
}

void loop()
{
    pollUserKey();
    updateStateMachine();

    if (gState == State::SPEAKER_TEST && millis() - gLastUiDrawMs >= kUiFrameIntervalMs) {
        gScreenDirty = true;
    }

    if (gScreenDirty && (gLastUiDrawMs == 0 || millis() - gLastUiDrawMs >= kUiFrameIntervalMs)) {
        redrawScreen();
        gScreenDirty = false;
        gLastUiDrawMs = millis();
    }

    delay(2);
}
