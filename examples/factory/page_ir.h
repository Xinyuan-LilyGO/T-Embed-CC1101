#pragma once

namespace page_ir {

namespace {
struct IrPreset {
    const char* name;
    const char* protocolName;
    decode_type_t protocol;
    uint64_t value;
    uint16_t bits;
};

struct UserHoldState {
    bool prevPressed = false;
    bool longHandled = false;
    uint32_t pressedAtMs = 0;
};

constexpr IrPreset kPresets[] = {
    {"NEC A",   "NEC",     NEC,     0x20DF10EFULL, 32},
    {"NEC B",   "NEC",     NEC,     0x20DF40BFULL, 32},
    {"Sony12",  "SONY",    SONY,    0x00000A90ULL, 12},
    {"Samsung", "SAMSUNG", SAMSUNG, 0xE0E040BFULL, 32},
    {"RC5",     "RC5",     RC5,     0x00000010ULL, 12},
};
constexpr uint8_t kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);
constexpr uint32_t kFrameMs = 80;
constexpr uint32_t kLoopbackIntervalMs = 1500;
constexpr uint32_t kEchoWindowMs = 250;
constexpr uint32_t kLoopbackHoldMs = 1200;

IRsend irsend(BOARD_IR_TX);
IRrecv irrecv(BOARD_IR_RX, 1024, 50, true);
decode_results results;

uint8_t presetIndex = 0;
uint32_t txCount = 0;
uint32_t rxCount = 0;
uint32_t lastTxMs = 0;
uint32_t lastRxMs = 0;
uint32_t lastLoopbackMs = 0;
String lastRx = "-";
String lastTx = "-";
bool loopbackMode = false;
bool backFocused = false;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
uint32_t lastAgeTickMs = 0;
int32_t encSnapshot = 0;
UserHoldState userHold;

bool sendPreset(const IrPreset& preset)
{
    switch (preset.protocol) {
        case NEC:
            irsend.sendNEC(preset.value, preset.bits);
            return true;
        case SONY:
            irsend.sendSony(preset.value, preset.bits, 2);
            return true;
        case SAMSUNG:
            irsend.sendSAMSUNG(preset.value, preset.bits);
            return true;
        case RC5:
            irsend.sendRC5(preset.value, preset.bits);
            return true;
        default:
            return false;
    }
}

void sendCurrentPreset()
{
    const IrPreset& preset = kPresets[presetIndex];
    if (!sendPreset(preset)) {
        Serial.println(F("[IR] Unsupported preset protocol."));
        return;
    }

    txCount++;
    lastTxMs = millis();
    lastTx = String(preset.name) + " 0x" + uint64ToString(preset.value, 16);
    Serial.println(String("[IR] TX ") + lastTx);
    screenDirty = true;
}

void cyclePreset()
{
    presetIndex = static_cast<uint8_t>((presetIndex + 1U) % kPresetCount);
    Serial.print(F("[IR] Preset -> "));
    Serial.println(kPresets[presetIndex].name);
    screenDirty = true;
}

void toggleLoopback()
{
    loopbackMode = !loopbackMode;
    lastLoopbackMs = 0;
    Serial.print(F("[IR] Loopback -> "));
    Serial.println(loopbackMode ? F("ON") : F("OFF"));
    screenDirty = true;
}

void pollRx()
{
    if (!irrecv.decode(&results)) {
        return;
    }

    const uint32_t now = millis();
    const bool selfEcho = (lastTxMs != 0U) && ((now - lastTxMs) < kEchoWindowMs);
    if (selfEcho && !loopbackMode) {
        irrecv.resume();
        return;
    }

    rxCount++;
    lastRxMs = now;
    lastRx = String(typeToString(results.decode_type, results.repeat)) + " 0x" + uint64ToString(results.value, 16);
    Serial.println(String("[IR] RX ") + lastRx + (selfEcho ? " (loopback)" : ""));
    irrecv.resume();
    screenDirty = true;
}

void pollLoopback()
{
    if (!loopbackMode) {
        return;
    }

    const uint32_t now = millis();
    if ((now - lastLoopbackMs) >= kLoopbackIntervalMs) {
        lastLoopbackMs = now;
        sendCurrentPreset();
    }
}

void updateUserControl()
{
    if (g.usrBtn.event) {
        g.usrBtn.event = false;
    }

    const bool pressed = g.usrBtn.pressed;
    const uint32_t now = millis();

    if (pressed && !userHold.prevPressed) {
        userHold.pressedAtMs = now;
        userHold.longHandled = false;
    } else if (pressed && !userHold.longHandled && (now - userHold.pressedAtMs) >= kLoopbackHoldMs) {
        userHold.longHandled = true;
        if (!backFocused) {
            toggleLoopback();
        }
    } else if (!pressed && userHold.prevPressed && !userHold.longHandled) {
        if (!backFocused) {
            cyclePreset();
        }
    }

    userHold.prevPressed = pressed;
}

String ageText(const uint32_t ms)
{
    if (ms == 0) {
        return "-";
    }
    return String((millis() - ms) / 1000U) + "s";
}
}  // namespace

void init()
{
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    lastAgeTickMs = 0;
    encSnapshot = g.encRaw;
    presetIndex = 0;
    txCount = 0;
    rxCount = 0;
    lastTxMs = 0;
    lastRxMs = 0;
    lastLoopbackMs = 0;
    lastRx = "-";
    lastTx = "-";
    loopbackMode = false;
    userHold = {};
    userHold.prevPressed = g.usrBtn.pressed;

    irsend.begin();
    irrecv.enableIRIn();
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    updateUserControl();

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        if (!loopbackMode) {
            sendCurrentPreset();
        }
    }

    pollRx();
    pollLoopback();

    if ((millis() - lastAgeTickMs) >= 1000U) {
        lastAgeTickMs = millis();
        screenDirty = true;
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    tft.fillScreen(kUiBg);
    drawPageHeader("IR TX / RX", TFT_ORANGE);
    drawPageFooter("USER=preset/loop  BOOT=send", backFocused);

    const IrPreset& preset = kPresets[presetIndex];
    drawCard(8, 34, 144, 36, "Preset", String(presetIndex + 1) + "/" + kPresetCount + " " + preset.name, TFT_YELLOW);
    drawCard(160, 34, 152, 36, "Mode", loopbackMode ? "Loopback" : "Manual", loopbackMode ? TFT_GREEN : TFT_CYAN);
    drawCard(8, 76, 200, 34, "Last TX", lastTx, TFT_WHITE);
    drawCard(216, 76, 96, 34, "TX age", ageText(lastTxMs), TFT_ORANGE);
    drawCard(8, 116, 200, 34, "Last RX", lastRx, TFT_WHITE);
    drawCard(216, 116, 96, 34, "RX/TX", String(rxCount) + "/" + String(txCount), TFT_GREEN);

    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit()
{
    irrecv.disableIRIn();
}

}  // namespace page_ir
