#pragma once

namespace page_mic {

namespace {
enum class State : uint8_t {
    InitMic = 0,
    MicTest,
    InitSpeaker,
    SpeakerTest,
    InitLoopback,
    LoopbackTest,
    DonePass,
    DoneFail,
};

enum class StageStatus : uint8_t {
    Pending = 0,
    Active,
    Pass,
    Fail,
};

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
constexpr uint32_t kFrameMs = 33;

State currentState = State::InitMic;
uint32_t stateEnteredAtMs = 0;
bool screenDirty = true;
bool backFocused = false;
uint32_t lastDrawMs = 0;
int32_t encSnapshot = 0;

bool micDriverOk = false;
bool spkDriverOk = false;
bool micOk = false;
bool spkOk = false;
bool loopOk = false;
bool micStarted = false;
bool spkStarted = false;
bool loopStarted = false;

uint16_t vuRms = 0;
uint16_t vuPeak = 0;
uint32_t vuPeakDecayMs = 0;

uint8_t freqIndex = 0;
uint16_t tonePhase = 0;
uint32_t freqLastChangeMs = 0;

int16_t loopBuf[kLoopBufSamples];
size_t loopWrite = 0;
size_t loopRead = 0;
bool loopFilled = false;

void setState(const State state)
{
    currentState = state;
    stateEnteredAtMs = millis();
    screenDirty = true;
}

uint16_t computeRms(const int16_t* buffer, const size_t count)
{
    if (count == 0) {
        return 0;
    }

    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        const int32_t sample = buffer[i];
        sum += sample * sample;
    }
    return static_cast<uint16_t>(sqrt(static_cast<double>(sum) / count));
}

void fillTone(int16_t* out, const size_t count, const float freq, uint16_t& phase)
{
    const uint16_t increment = static_cast<uint16_t>(freq * 65536.0f / kSampleRate);
    for (size_t i = 0; i < count; ++i) {
        out[i] = static_cast<int16_t>(kToneAmplitude * sinf(phase * (kTwoPi / 65536.0f)));
        phase += increment;
    }
}

void writeSpeakerSamples(const int16_t* samples, const size_t count)
{
    if (!spkDriverOk || count == 0) {
        return;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(samples);
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
    fillTone(buffer, kSpeakerChunkSamples, kToneFreqs[freqIndex], tonePhase);
    writeSpeakerSamples(buffer, kSpeakerChunkSamples);
}

bool initMic()
{
    if (micDriverOk) {
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
    micDriverOk = true;
    return true;
}

void deinitMic()
{
    if (!micDriverOk) {
        return;
    }
    i2s_driver_uninstall(kMicPort);
    micDriverOk = false;
}

bool initSpeaker()
{
    if (spkDriverOk) {
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
    spkDriverOk = true;
    return true;
}

void deinitSpeaker()
{
    if (!spkDriverOk) {
        return;
    }
    i2s_zero_dma_buffer(kSpkPort);
    i2s_driver_uninstall(kSpkPort);
    spkDriverOk = false;
}

void resetTestState()
{
    deinitMic();
    deinitSpeaker();

    micOk = false;
    spkOk = false;
    loopOk = false;
    micStarted = false;
    spkStarted = false;
    loopStarted = false;
    vuRms = 0;
    vuPeak = 0;
    vuPeakDecayMs = 0;
    freqIndex = 0;
    tonePhase = 0;
    loopWrite = 0;
    loopRead = 0;
    loopFilled = false;

    setState(State::InitMic);
}

void updateStateMachine()
{
    const uint32_t now = millis();

    switch (currentState) {
        case State::InitMic:
            micStarted = true;
            vuRms = 0;
            vuPeak = 0;
            vuPeakDecayMs = now;
            if (!initMic()) {
                Serial.println(F("[MIC] Mic I2S init failed."));
                setState(State::DoneFail);
            } else {
                Serial.println(F("[MIC] Starting microphone test."));
                setState(State::MicTest);
            }
            break;

        case State::MicTest: {
            int16_t buffer[kBufSamples];
            size_t bytesRead = 0;
            i2s_read(kMicPort, buffer, sizeof(buffer), &bytesRead, 0);
            const size_t sampleCount = bytesRead / sizeof(int16_t);
            if (sampleCount > 0) {
                const uint16_t rms = computeRms(buffer, sampleCount);
                vuRms = rms;
                if (rms > vuPeak) {
                    vuPeak = rms;
                    vuPeakDecayMs = now;
                }
                if (now - vuPeakDecayMs > 800U) {
                    vuPeak = (vuPeak * 15U) / 16U;
                }
                if (rms > kMicSignalThreshold) {
                    micOk = true;
                }
                screenDirty = true;
            }
            if ((now - stateEnteredAtMs) >= kMicTestDurationMs) {
                deinitMic();
                Serial.println(micOk ? F("[MIC] Mic test PASS") : F("[MIC] Mic test FAIL"));
                setState(State::InitSpeaker);
            }
            break;
        }

        case State::InitSpeaker:
            spkStarted = true;
            vuRms = 0;
            vuPeak = 0;
            vuPeakDecayMs = now;
            freqIndex = 0;
            tonePhase = 0;
            freqLastChangeMs = now;
            if (!initSpeaker()) {
                Serial.println(F("[MIC] Speaker I2S init failed."));
                spkOk = false;
                setState(State::InitLoopback);
            } else {
                spkOk = true;
                for (uint8_t i = 0; i < kSpeakerWarmupChunks; ++i) {
                    writeToneChunk();
                }
                Serial.println(F("[MIC] Starting speaker tone sweep."));
                setState(State::SpeakerTest);
            }
            break;

        case State::SpeakerTest:
            writeToneChunk();
            if ((now - freqLastChangeMs) >= kSpkFreqDwellMs) {
                freqIndex = static_cast<uint8_t>((freqIndex + 1U) % 3U);
                freqLastChangeMs = now;
                screenDirty = true;
            }
            if ((now - stateEnteredAtMs) >= kSpkFreqDwellMs * 3U) {
                deinitSpeaker();
                Serial.println(F("[MIC] Speaker test done."));
                setState(State::InitLoopback);
            }
            break;

        case State::InitLoopback:
            loopStarted = true;
            loopWrite = 0;
            loopRead = 0;
            loopFilled = false;
            vuRms = 0;
            vuPeak = 0;
            vuPeakDecayMs = now;
            if (!initMic()) {
                Serial.println(F("[MIC] Loopback mic init failed."));
                setState(State::DoneFail);
                break;
            }
            if (!initSpeaker()) {
                Serial.println(F("[MIC] Loopback speaker init failed."));
                deinitMic();
                setState(State::DoneFail);
                break;
            }
            Serial.println(F("[MIC] Starting loopback test."));
            setState(State::LoopbackTest);
            break;

        case State::LoopbackTest: {
            int16_t inputBuffer[kBufSamples];
            size_t bytesRead = 0;
            i2s_read(kMicPort, inputBuffer, sizeof(inputBuffer), &bytesRead, 0);
            const size_t sampleCount = bytesRead / sizeof(int16_t);

            for (size_t i = 0; i < sampleCount; ++i) {
                loopBuf[loopWrite] = inputBuffer[i];
                loopWrite = (loopWrite + 1U) % kLoopBufSamples;
                if (loopWrite == 0U) {
                    loopFilled = true;
                }
            }

            if (sampleCount > 0) {
                vuRms = computeRms(inputBuffer, sampleCount);
                if (vuRms > vuPeak) {
                    vuPeak = vuRms;
                    vuPeakDecayMs = now;
                }
                if (now - vuPeakDecayMs > 800U) {
                    vuPeak = (vuPeak * 15U) / 16U;
                }
                if (vuRms > kMicSignalThreshold) {
                    loopOk = true;
                }
                screenDirty = true;
            }

            if (loopFilled) {
                int16_t outputBuffer[kBufSamples];
                for (size_t i = 0; i < kBufSamples; ++i) {
                    outputBuffer[i] = loopBuf[loopRead];
                    loopRead = (loopRead + 1U) % kLoopBufSamples;
                }
                writeSpeakerSamples(outputBuffer, kBufSamples);
            }

            if ((now - stateEnteredAtMs) >= kLoopbackDurationMs) {
                deinitMic();
                deinitSpeaker();
                const bool allPass = micOk && spkOk && loopOk;
                Serial.println(allPass ? F("[MIC] Loopback PASS") : F("[MIC] Loopback FAIL"));
                setState(allPass ? State::DonePass : State::DoneFail);
            }
            break;
        }

        case State::DonePass:
        case State::DoneFail:
        default:
            break;
    }
}

const char* stateLabel()
{
    switch (currentState) {
        case State::InitMic:      return "INIT MIC";
        case State::MicTest:      return "MIC TEST";
        case State::InitSpeaker:  return "INIT SPK";
        case State::SpeakerTest:  return "SPK TEST";
        case State::InitLoopback: return "INIT LOOP";
        case State::LoopbackTest: return "LOOP TEST";
        case State::DonePass:     return "PASS";
        case State::DoneFail:     return "FAIL";
        default:                  return "?";
    }
}

uint16_t stateAccent()
{
    switch (currentState) {
        case State::InitMic:
        case State::MicTest:
            return TFT_CYAN;
        case State::InitSpeaker:
        case State::SpeakerTest:
            return TFT_YELLOW;
        case State::InitLoopback:
        case State::LoopbackTest:
            return TFT_ORANGE;
        case State::DonePass:
            return TFT_GREEN;
        case State::DoneFail:
            return TFT_RED;
        default:
            return TFT_WHITE;
    }
}

String stateHint()
{
    switch (currentState) {
        case State::InitMic:      return "Starting PDM microphone";
        case State::MicTest:      return "Make some noise near MIC";
        case State::InitSpeaker:  return "Preparing I2S speaker";
        case State::SpeakerTest:  return String("Tone ") + String(kToneFreqs[freqIndex], 0) + " Hz";
        case State::InitLoopback: return "Preparing loopback path";
        case State::LoopbackTest: return "Speak to hear delayed playback";
        case State::DonePass:     return "Audio self-test passed";
        case State::DoneFail:     return "Check MIC / speaker path";
        default:                  return "";
    }
}

const char* stageText(const StageStatus status)
{
    switch (status) {
        case StageStatus::Active: return "RUN";
        case StageStatus::Pass:   return "PASS";
        case StageStatus::Fail:   return "FAIL";
        case StageStatus::Pending:
        default:                  return "WAIT";
    }
}

uint16_t stageColor(const StageStatus status)
{
    switch (status) {
        case StageStatus::Active: return TFT_YELLOW;
        case StageStatus::Pass:   return TFT_GREEN;
        case StageStatus::Fail:   return TFT_RED;
        case StageStatus::Pending:
        default:                  return TFT_DARKGREY;
    }
}

StageStatus micStage()
{
    if (currentState == State::InitMic || currentState == State::MicTest) {
        return StageStatus::Active;
    }
    if (!micStarted) {
        return StageStatus::Pending;
    }
    return micOk ? StageStatus::Pass : StageStatus::Fail;
}

StageStatus spkStage()
{
    if (currentState == State::InitSpeaker || currentState == State::SpeakerTest) {
        return StageStatus::Active;
    }
    if (!spkStarted) {
        return StageStatus::Pending;
    }
    return spkOk ? StageStatus::Pass : StageStatus::Fail;
}

StageStatus loopStage()
{
    if (currentState == State::InitLoopback || currentState == State::LoopbackTest) {
        return StageStatus::Active;
    }
    if (!loopStarted) {
        return StageStatus::Pending;
    }
    return loopOk ? StageStatus::Pass : StageStatus::Fail;
}
}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;
    resetTestState();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    const bool userPressed = takeUserButton();
    const bool bootPressed = takeEncoderButton();
    const bool done = currentState == State::DonePass || currentState == State::DoneFail;

    if (bootPressed) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        if (done) {
            resetTestState();
        }
    }

    if (userPressed && done && !backFocused) {
        resetTestState();
    }

    updateStateMachine();
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    tft.fillScreen(kUiBg);
    drawPageHeader("MIC", stateAccent());
    drawPageFooter("BOOT rerun after done", backFocused);

    drawCard(8, 34, 96, 30, "MIC", stageText(micStage()), stageColor(micStage()));
    drawCard(112, 34, 96, 30, "SPK", stageText(spkStage()), stageColor(spkStage()));
    drawCard(216, 34, 96, 30, "LOOP", stageText(loopStage()), stageColor(loopStage()));
    drawCard(8, 72, 304, 42, stateLabel(), stateHint(), stateAccent());
    drawCard(8, 122, 96, 30, "RMS", String(vuRms), TFT_WHITE);
    drawCard(112, 122, 96, 30, "Peak", String(vuPeak), TFT_CYAN);
    drawCard(216, 122, 96, 30, "Info",
             currentState == State::SpeakerTest ? String(kToneFreqs[freqIndex], 0) + "Hz"
             : (currentState == State::DonePass ? "OK"
             : (currentState == State::DoneFail ? "CHECK"
             : (vuRms > kMicSignalThreshold ? "LIVE" : "WAIT"))),
             stateAccent());

    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    deinitMic();
    deinitSpeaker();
}

}  // namespace page_mic
