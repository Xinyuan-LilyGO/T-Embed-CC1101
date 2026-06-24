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

struct StageStyle {
    uint16_t fill;
    uint16_t border;
    uint16_t label;
    uint16_t tag;
    const char* tagText;
};

constexpr int16_t kUiMargin = 8;
constexpr uint32_t kFrameMs = 33;
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

TFT_eSprite canvas(&tft);
State currentState = State::InitMic;
uint32_t stateEnteredAtMs = 0;
bool screenDirty = true;
bool backFocused = false;
bool canvasReady = false;
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
            Serial.println(F("[AUD] Starting mic test (5 s - make some noise)..."));
            if (!initMic()) {
                Serial.println(F("[AUD] Mic I2S init failed."));
                setState(State::DoneFail);
            } else {
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
                Serial.print(F("[AUD] Mic test "));
                Serial.println(micOk ? F("PASS") : F("FAIL (no signal)"));
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
            Serial.println(F("[AUD] Starting speaker test (3 tones)..."));
            if (!initSpeaker()) {
                Serial.println(F("[AUD] Speaker I2S init failed."));
                spkOk = false;
                setState(State::InitLoopback);
            } else {
                spkOk = true;
                for (uint8_t i = 0; i < kSpeakerWarmupChunks; ++i) {
                    writeToneChunk();
                }
                setState(State::SpeakerTest);
            }
            break;

        case State::SpeakerTest:
            writeToneChunk();
            if ((now - freqLastChangeMs) >= kSpkFreqDwellMs) {
                freqIndex = static_cast<uint8_t>((freqIndex + 1U) % 3U);
                freqLastChangeMs = now;
                screenDirty = true;
                Serial.print(F("[AUD] Tone -> "));
                Serial.print(kToneFreqs[freqIndex]);
                Serial.println(F(" Hz"));
            }
            if ((now - stateEnteredAtMs) >= (kSpkFreqDwellMs * 3U)) {
                deinitSpeaker();
                Serial.println(F("[AUD] Speaker test done."));
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
            Serial.println(F("[AUD] Starting loopback test (8 s - speak into mic)..."));
            if (!initMic()) {
                Serial.println(F("[AUD] Loopback mic init failed."));
                setState(State::DoneFail);
                break;
            }
            if (!initSpeaker()) {
                Serial.println(F("[AUD] Loopback speaker init failed."));
                deinitMic();
                setState(State::DoneFail);
                break;
            }
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
                Serial.print(F("[AUD] Loopback test "));
                Serial.println(loopOk ? F("PASS") : F("FAIL (no mic signal)"));
                setState((micOk && spkOk && loopOk) ? State::DonePass : State::DoneFail);
            }
            break;
        }

        case State::DonePass:
        case State::DoneFail:
        default:
            break;
    }
}

bool testDone()
{
    return currentState == State::DonePass || currentState == State::DoneFail;
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
            return TFT_DARKGREY;
    }
}

const char* stateTitle()
{
    switch (currentState) {
        case State::InitMic:      return "Initializing microphone";
        case State::MicTest:      return "Microphone capture test";
        case State::InitSpeaker:  return "Preparing speaker output";
        case State::SpeakerTest:  return "Speaker tone sweep";
        case State::InitLoopback: return "Configuring loopback";
        case State::LoopbackTest: return "Mic to speaker loopback";
        case State::DonePass:     return "Audio self-check passed";
        case State::DoneFail:     return "Audio self-check failed";
        default:                  return "Audio self-check";
    }
}

const char* stateHint()
{
    switch (currentState) {
        case State::InitMic:      return "Starting the PDM mic path...";
        case State::MicTest:      return "Make some noise close to the microphone.";
        case State::InitSpeaker:  return "Enabling I2S speaker output...";
        case State::SpeakerTest:  return "Listen for the three test tones.";
        case State::InitLoopback: return "Bringing mic and speaker online together...";
        case State::LoopbackTest: return "Speak now. You should hear delayed playback.";
        case State::DonePass:     return "All three stages completed successfully.";
        case State::DoneFail:     return "Check mic, speaker and power path, then rerun.";
        default:                  return "";
    }
}

uint32_t activeStageDurationMs()
{
    switch (currentState) {
        case State::MicTest:      return kMicTestDurationMs;
        case State::SpeakerTest:  return kSpkFreqDwellMs * 3U;
        case State::LoopbackTest: return kLoopbackDurationMs;
        default:                  return 0;
    }
}

uint32_t activeStageElapsedMs()
{
    const uint32_t duration = activeStageDurationMs();
    if (duration == 0) {
        return 0;
    }
    const uint32_t elapsed = millis() - stateEnteredAtMs;
    return elapsed > duration ? duration : elapsed;
}

bool hasInputMeter()
{
    return currentState == State::MicTest ||
           currentState == State::LoopbackTest ||
           currentState == State::DonePass ||
           currentState == State::DoneFail;
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

StageStyle stageStyle(const StageStatus status, const uint16_t accent)
{
    switch (status) {
        case StageStatus::Active:
            return {kColorCard, accent, TFT_WHITE, accent, "RUN"};
        case StageStatus::Pass:
            return {kColorPassBg, TFT_GREEN, TFT_WHITE, TFT_GREEN, "PASS"};
        case StageStatus::Fail:
            return {kColorFailBg, TFT_RED, TFT_WHITE, TFT_RED, "FAIL"};
        case StageStatus::Pending:
        default:
            return {kColorCard, kColorPanelEdge, TFT_LIGHTGREY, TFT_DARKGREY, "WAIT"};
    }
}

uint16_t meterBarColor(const uint16_t width, const uint16_t fullWidth)
{
    if (width < (fullWidth * 60U / 100U)) {
        return TFT_GREEN;
    }
    if (width < (fullWidth * 85U / 100U)) {
        return TFT_YELLOW;
    }
    return TFT_RED;
}

const char* levelLabel(const uint16_t rms)
{
    if (rms < 100) {
        return "SILENT";
    }
    if (rms < 500) {
        return "LOW";
    }
    if (rms < 2000) {
        return "MED";
    }
    return "LOUD";
}

uint16_t levelColor(const uint16_t rms)
{
    if (rms < 100) {
        return TFT_DARKGREY;
    }
    if (rms < 500) {
        return TFT_CYAN;
    }
    if (rms < 2000) {
        return TFT_GREEN;
    }
    return TFT_RED;
}

template <typename Canvas>
void drawHeader(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderH, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("Mic & Speaker Test", kUiMargin, 5, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("16 kHz audio", gfx.width() - kUiMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas& gfx)
{
    const int16_t y = gfx.height() - kFooterH;
    gfx.fillRect(0, y, gfx.width(), kFooterH, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    if (backFocused) {
        gfx.drawString("BOOT back", kUiMargin, y + 4, 1);
    } else if (testDone()) {
        gfx.drawString("USER/BOOT rerun", kUiMargin, y + 4, 1);
    } else {
        gfx.drawString("Audio self-test running", kUiMargin, y + 4, 1);
    }
}

template <typename Canvas>
void drawBackButton(Canvas& gfx, const bool selected)
{
    const int16_t w = 58;
    const int16_t h = 14;
    const int16_t x = gfx.width() - w - 6;
    const int16_t y = gfx.height() - kFooterH + 2;
    const uint16_t bg = selected ? TFT_WHITE : TFT_DARKGREY;
    const uint16_t fg = selected ? TFT_BLACK : TFT_LIGHTGREY;

    gfx.fillRoundRect(x, y, w, h, 5, bg);
    gfx.drawRoundRect(x, y, w, h, 5, selected ? TFT_YELLOW : 0x52AA);
    gfx.setTextColor(fg, bg);
    gfx.drawCentreString("BACK", x + w / 2, y + 3, 1);
}

template <typename Canvas>
void drawStageCard(Canvas& gfx, const int16_t x, const char* label, const StageStatus status, const uint16_t accent)
{
    const StageStyle style = stageStyle(status, accent);
    gfx.fillRoundRect(x, kStageCardY, kStageCardW, kStageCardH, 6, style.fill);
    gfx.drawRoundRect(x, kStageCardY, kStageCardW, kStageCardH, 6, style.border);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(style.label, style.fill);
    gfx.drawString(label, x + 8, kStageCardY + 6, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(style.tag, style.fill);
    gfx.drawString(style.tagText, x + kStageCardW - 8, kStageCardY + 9, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawStageRow(Canvas& gfx)
{
    drawStageCard(gfx, kUiMargin, "MIC", micStage(), TFT_CYAN);
    drawStageCard(gfx, kUiMargin + kStageCardW + kStageGap, "SPK", spkStage(), TFT_YELLOW);
    drawStageCard(gfx, kUiMargin + (kStageCardW + kStageGap) * 2, "LOOP", loopStage(), TFT_ORANGE);
}

template <typename Canvas>
void drawProgressBar(Canvas& gfx, const int16_t x, const int16_t y, const int16_t w, const int16_t h)
{
    gfx.drawRect(x, y, w, h, kColorPanelEdge);
    gfx.fillRect(x + 1, y + 1, w - 2, h - 2, kColorMeterBg);

    const uint32_t duration = activeStageDurationMs();
    if (duration == 0 || w <= 2 || h <= 2) {
        return;
    }

    const int16_t fillW = static_cast<int16_t>((static_cast<uint32_t>(w - 2) * activeStageElapsedMs()) / duration);
    if (fillW > 0) {
        gfx.fillRect(x + 1, y + 1, fillW, h - 2, stateAccent());
    }
}

template <typename Canvas>
void drawVuBar(Canvas& gfx, const int16_t x, const int16_t y, const int16_t w, const int16_t h)
{
    gfx.drawRect(x - 1, y - 1, w + 2, h + 2, kColorPanelEdge);
    gfx.fillRect(x, y, w, h, kColorMeterBg);

    int16_t fillW = static_cast<int16_t>((static_cast<uint32_t>(vuRms) * w) / 8000U);
    if (fillW > w) {
        fillW = w;
    }
    if (fillW > 0) {
        gfx.fillRect(x, y, fillW, h, meterBarColor(fillW, w));
    }

    int16_t peakX = static_cast<int16_t>((static_cast<uint32_t>(vuPeak) * w) / 8000U);
    if (peakX > w) {
        peakX = w;
    }
    if (peakX > 0) {
        gfx.drawFastVLine(x + peakX - 1, y, h, TFT_WHITE);
    }
}

template <typename Canvas>
void drawActivePanel(Canvas& gfx)
{
    gfx.fillRoundRect(kPanelX, kPanelY, kPanelW, kPanelH, 8, kColorPanel);
    gfx.drawRoundRect(kPanelX, kPanelY, kPanelW, kPanelH, 8, stateAccent());

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, kColorPanel);
    gfx.drawString(stateTitle(), kPanelX + 10, kPanelY + 7, 2);
    drawProgressBar(gfx, kPanelX + kPanelW - 108, kPanelY + 10, 96, 8);

    gfx.setTextColor(TFT_LIGHTGREY, kColorPanel);
    gfx.drawString(stateHint(), kPanelX + 10, kPanelY + 24, 1);

    if (currentState == State::MicTest || currentState == State::LoopbackTest) {
        drawVuBar(gfx, kPanelX + 12, kPanelY + 36, kPanelW - 24, 10);
        return;
    }

    if (currentState == State::SpeakerTest) {
        char freqBuf[16];
        snprintf(freqBuf, sizeof(freqBuf), "%.0f Hz", kToneFreqs[freqIndex]);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(TFT_YELLOW, kColorPanel);
        gfx.drawString(freqBuf, kPanelX + kPanelW / 2, kPanelY + 35, 4);
        gfx.setTextDatum(TL_DATUM);
        return;
    }

    if (testDone()) {
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(stateAccent(), kColorPanel);
        gfx.drawString(currentState == State::DonePass ? "PASS" : "FAIL",
                       kPanelX + kPanelW / 2,
                       kPanelY + 35,
                       4);
        gfx.setTextDatum(TL_DATUM);
    }
}

template <typename Canvas>
void drawMetricCard(Canvas& gfx, const int16_t x, const char* title, const char* value,
                    const uint16_t valueColor, const uint16_t borderColor)
{
    gfx.fillRoundRect(x, kMetricY, kMetricW, kMetricH, 6, kColorCard);
    gfx.drawRoundRect(x, kMetricY, kMetricW, kMetricH, 6, borderColor);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_LIGHTGREY, kColorCard);
    gfx.drawString(title, x + 6, kMetricY + 5, 1);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(valueColor, kColorCard);
    gfx.drawString(value, x + kMetricW - 6, kMetricY + 4, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawMetricRow(Canvas& gfx)
{
    char rmsBuf[12];
    char peakBuf[12];
    char infoBuf[16];

    if (hasInputMeter()) {
        snprintf(rmsBuf, sizeof(rmsBuf), "%u", vuRms);
        snprintf(peakBuf, sizeof(peakBuf), "%u", vuPeak);
    } else {
        snprintf(rmsBuf, sizeof(rmsBuf), "--");
        snprintf(peakBuf, sizeof(peakBuf), "--");
    }

    uint16_t infoColor = TFT_DARKGREY;
    if (currentState == State::SpeakerTest) {
        snprintf(infoBuf, sizeof(infoBuf), "%.0fHz", kToneFreqs[freqIndex]);
        infoColor = TFT_YELLOW;
    } else if (currentState == State::DonePass) {
        snprintf(infoBuf, sizeof(infoBuf), "OK");
        infoColor = TFT_GREEN;
    } else if (currentState == State::DoneFail) {
        snprintf(infoBuf, sizeof(infoBuf), "CHECK");
        infoColor = TFT_RED;
    } else if (hasInputMeter()) {
        snprintf(infoBuf, sizeof(infoBuf), "%s", levelLabel(vuRms));
        infoColor = levelColor(vuRms);
    } else {
        snprintf(infoBuf, sizeof(infoBuf), "WAIT");
    }

    drawMetricCard(gfx, kUiMargin, "RMS", rmsBuf, TFT_WHITE, kColorPanelEdge);
    drawMetricCard(gfx, kUiMargin + kMetricW + kStageGap, "PEAK", peakBuf, TFT_WHITE, kColorPanelEdge);
    drawMetricCard(gfx, kUiMargin + (kMetricW + kStageGap) * 2, "INFO", infoBuf, infoColor, stateAccent());
}

template <typename Canvas>
void drawUi(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kColorBg);
    drawHeader(gfx);
    drawStageRow(gfx);
    drawActivePanel(gfx);
    drawMetricRow(gfx);
    drawFooter(gfx);
    drawBackButton(gfx, backFocused);
}

void redrawAll()
{
    board_prepare_display();
    if (canvasReady) {
        drawUi(canvas);
        canvas.pushSprite(0, 0);
    } else {
        drawUi(tft);
    }
    board_spi_deselect_all();
}
}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    canvasReady = false;
    lastDrawMs = 0;
    encSnapshot = g.encRaw;
    canvas.setColorDepth(16);
    canvasReady = (canvas.createSprite(tft.width(), tft.height()) != nullptr);
    if (!canvasReady) {
        Serial.println(F("[AUD] Sprite allocation failed, using direct TFT redraw."));
    }
    resetTestState();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    const bool userPressed = takeUserButton();
    const bool bootPressed = takeEncoderButton();
    const bool done = testDone();

    if (bootPressed) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        if (done) {
            Serial.println(F("[AUD] Restarting test..."));
            resetTestState();
        }
    }

    if (userPressed && done && !backFocused) {
        Serial.println(F("[AUD] Restarting test..."));
        resetTestState();
    }

    updateStateMachine();
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0U && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    redrawAll();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    canvas.deleteSprite();
    canvasReady = false;
    deinitMic();
    deinitSpeaker();
}

}  // namespace page_mic
