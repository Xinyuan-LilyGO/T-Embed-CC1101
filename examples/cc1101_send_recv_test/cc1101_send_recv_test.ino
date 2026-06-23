#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>

#include "utilities.h"

namespace {

struct FreqChoice {
  float mhz;
  const char* label;
};

constexpr FreqChoice kFreqChoices[] = {
  {315.0f, "315 MHz"},
  {433.92f, "433.92 MHz"},
  {868.0f, "868 MHz"},
};
constexpr uint8_t kFreqChoiceCount = sizeof(kFreqChoices) / sizeof(kFreqChoices[0]);

constexpr float kBitRateKbps = 1.2f;
constexpr float kRxBandwidthKHz = 58.0f;
constexpr float kFrequencyDeviationKHz = 5.2f;
constexpr int8_t kOutputPowerDbm = 10;
constexpr bool kUseOok = true;
constexpr uint8_t kSyncWordHigh = 0x01;
constexpr uint8_t kSyncWordLow = 0x23;
constexpr uint32_t kBurstIntervalMs = 1000;
constexpr char kDefaultTxPrefix[] = "T-Embed CC1101";

constexpr uint8_t kRotation = 3;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;
constexpr int16_t kMarginLeft = 8;
constexpr int16_t kRowFreq = 30;
constexpr int16_t kRowMode = 64;
constexpr int16_t kRowDivider = 86;
constexpr int16_t kRowInfo1 = 92;
constexpr int16_t kRowInfo2 = 108;
constexpr int16_t kRowInfo3 = 124;
constexpr int16_t kRowInfo4 = 140;

constexpr uint8_t kCcRegIocfg2 = 0x00;
constexpr uint8_t kCcRegIocfg0 = 0x02;
constexpr uint8_t kCcRegPktCtrl0 = 0x08;
constexpr uint8_t kCcRegMdmCfg2 = 0x12;
constexpr uint8_t kCcCmdSidle = 0x36;
constexpr uint8_t kCcCmdSfrx = 0x3A;
constexpr uint8_t kCcCmdSrx = 0x34;
constexpr uint8_t kCcGdoSerialDataAsync = 0x0D;
constexpr uint8_t kCcPktCtrl0AsyncSerial = 0x30;
constexpr float kSniffRxBwKHz = 270.0f;
constexpr float kSniffBitRateKbps = 50.0f;

constexpr uint8_t kLedBrightness = 10;
constexpr uint32_t kLedFlashMs = 200;

constexpr size_t kPulseRingSize = 256;
constexpr uint32_t kBurstSilenceUs = 5000;
constexpr uint32_t kMinValidPulseUs = 80;
constexpr uint32_t kMaxValidPulseUs = 20000;
constexpr uint32_t kSniffScreenIntervalMs = 250;

enum class RadioMode : uint8_t {
  Receive,
  BurstTransmit,
  SniffOok,
};

enum class LedEffect : uint8_t {
  None,
  RxFlash,
  TxFlash,
  SniffFlash,
};

struct PulseEvent {
  uint32_t durationUs;
};

SPIClass radioSPI(HSPI);
CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, RADIOLIB_NC, BOARD_LORA_IO2, radioSPI);
TFT_eSPI tft;
Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

volatile bool packetReceived = false;
volatile PulseEvent gPulseRing[kPulseRingSize];
volatile uint16_t gPulseHead = 0;
volatile uint16_t gPulseTail = 0;
volatile uint32_t gLastEdgeUs = 0;
volatile uint32_t gIsrEdgeCount = 0;

RadioMode currentMode = RadioMode::Receive;
uint8_t currentFreqIndex = 1;
unsigned long lastBurstAtMs = 0;
uint32_t burstCounter = 0;
uint32_t rxCounter = 0;
String burstPrefix = kDefaultTxPrefix;

String lastRxPayload;
float lastRxRssi = 0.0f;
uint8_t lastRxLqi = 0;
bool hasLastRx = false;
bool radioReady = false;
bool screenDirty = true;
bool needFullRedraw = true;
bool needSniffStatsRedraw = false;

LedEffect gLedEffect = LedEffect::None;
uint32_t gLedEffectUntilMs = 0;
bool gLedDirty = true;

uint32_t gTotalEdges = 0;
uint32_t gBurstCount = 0;
uint16_t gCurrentBurstPulses = 0;
uint32_t gCurrentBurstMinUs = 0;
uint32_t gCurrentBurstMaxUs = 0;
uint16_t gLastBurstPulses = 0;
uint32_t gLastBurstMinUs = 0;
uint32_t gLastBurstMaxUs = 0;
uint32_t gLastBurstShortUs = 0;
uint32_t gLastBurstLongUs = 0;
uint32_t gLastEdgeAtMs = 0;
uint32_t gLastSniffDrawMs = 0;
bool gInBurst = false;

inline float currentFrequencyMHz() {
  return kFreqChoices[currentFreqIndex].mhz;
}

inline const char* currentFrequencyLabel() {
  return kFreqChoices[currentFreqIndex].label;
}

const char* modeLabel(RadioMode mode) {
  switch (mode) {
    case RadioMode::Receive:
      return "RX";
    case RadioMode::BurstTransmit:
      return "TX-BURST";
    case RadioMode::SniffOok:
      return "SNIFF-OOK";
    default:
      return "?";
  }
}

void deselectSharedSpiDevices() {
  digitalWrite(DISPLAY_CS, HIGH);
  digitalWrite(BOARD_SD_CS, HIGH);
  digitalWrite(BOARD_LORA_CS, HIGH);
}

void initSharedSpiPins() {
  pinMode(DISPLAY_CS, OUTPUT);
  pinMode(BOARD_SD_CS, OUTPUT);
  pinMode(BOARD_LORA_CS, OUTPUT);
  deselectSharedSpiDevices();
}

void prepareDisplayBus() {
  deselectSharedSpiDevices();
}

void prepareRadioBus() {
  deselectSharedSpiDevices();
}

bool setRfPathForFrequencyIndex(uint8_t index) {
  pinMode(BOARD_LORA_SW1, OUTPUT);
  pinMode(BOARD_LORA_SW0, OUTPUT);

  switch (index) {
    case 0:
      digitalWrite(BOARD_LORA_SW1, HIGH);
      digitalWrite(BOARD_LORA_SW0, LOW);
      return true;
    case 1:
      digitalWrite(BOARD_LORA_SW1, HIGH);
      digitalWrite(BOARD_LORA_SW0, HIGH);
      return true;
    case 2:
      digitalWrite(BOARD_LORA_SW1, LOW);
      digitalWrite(BOARD_LORA_SW0, HIGH);
      return true;
    default:
      return false;
  }
}

#if defined(ESP32)
void IRAM_ATTR onPacketReceived() {
  packetReceived = true;
}

void IRAM_ATTR onSniffEdge() {
  const uint32_t now = micros();
  const uint32_t dur = now - gLastEdgeUs;
  gLastEdgeUs = now;

  const uint16_t next = (gPulseHead + 1U) % kPulseRingSize;
  if (next != gPulseTail) {
    gPulseRing[gPulseHead].durationUs = dur;
    gPulseHead = next;
  }
  gIsrEdgeCount++;
}
#else
void onPacketReceived() {
  packetReceived = true;
}

void onSniffEdge() {
  const uint32_t now = micros();
  const uint32_t dur = now - gLastEdgeUs;
  gLastEdgeUs = now;

  const uint16_t next = (gPulseHead + 1U) % kPulseRingSize;
  if (next != gPulseTail) {
    gPulseRing[gPulseHead].durationUs = dur;
    gPulseHead = next;
  }
  gIsrEdgeCount++;
}
#endif

void triggerLedEffect(LedEffect effect) {
  gLedEffect = effect;
  gLedEffectUntilMs = millis() + kLedFlashMs;
  gLedDirty = true;
}

void setStripColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < WS2812_NUM_LEDS; ++i) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void updateLeds() {
  if (!gLedDirty) {
    return;
  }
  gLedDirty = false;

  if ((gLedEffect == LedEffect::None) || (millis() > gLedEffectUntilMs)) {
    gLedEffect = LedEffect::None;
    setStripColor(0, 0, 0);
    return;
  }

  switch (gLedEffect) {
    case LedEffect::RxFlash:
      setStripColor(0, kLedBrightness, 0);
      break;
    case LedEffect::TxFlash:
      setStripColor(0, 0, kLedBrightness);
      break;
    case LedEffect::SniffFlash:
      setStripColor(kLedBrightness, 0, kLedBrightness);
      break;
    default:
      setStripColor(0, 0, 0);
      break;
  }
}

void checkLedTimeout() {
  if ((gLedEffect != LedEffect::None) && (millis() > gLedEffectUntilMs)) {
    gLedDirty = true;
  }
}

void drawHeader() {
  tft.fillRect(0, 0, tft.width(), kHeaderHeight, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString("CC1101 Send / Recv", kMarginLeft, 6, 2);
}

void drawFooter(const char* msg) {
  const int16_t y = tft.height() - kFooterHeight;
  tft.fillRect(0, y, tft.width(), kFooterHeight, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.drawString(msg, kMarginLeft, y + 3, 1);
}

void drawFreqRow() {
  tft.fillRect(0, kRowFreq, tft.width(), 28, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("FREQ:", kMarginLeft, kRowFreq + 8, 1);

  uint16_t color = TFT_ORANGE;
  if (currentFreqIndex == 0) {
    color = TFT_GREEN;
  } else if (currentFreqIndex == 1) {
    color = TFT_YELLOW;
  }

  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(currentFrequencyLabel(), kMarginLeft + 50, kRowFreq, 4);
}

void drawModeRow() {
  tft.fillRect(0, kRowMode, tft.width(), 18, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("MODE:", kMarginLeft, kRowMode + 2, 1);

  uint16_t color = TFT_WHITE;
  const char* label = "?";
  switch (currentMode) {
    case RadioMode::Receive:
      color = TFT_GREEN;
      label = "RX";
      break;
    case RadioMode::BurstTransmit:
      color = TFT_ORANGE;
      label = "TX BURST";
      break;
    case RadioMode::SniffOok:
      color = TFT_MAGENTA;
      label = "SNIFF OOK";
      break;
    default:
      break;
  }

  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(label, kMarginLeft + 50, kRowMode, 2);
}

void drawDivider() {
  tft.drawFastHLine(0, kRowDivider, tft.width(), TFT_DARKGREY);
}

void drawReceiveRows() {
  tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Last RX:", kMarginLeft, kRowInfo1, 1);

  if (!hasLastRx) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("(none)", kMarginLeft + 56, kRowInfo1, 1);
    return;
  }

  String payload = lastRxPayload;
  if (payload.length() > 38) {
    payload = payload.substring(0, 35) + "...";
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(payload, kMarginLeft, kRowInfo2, 1);

  char line[48];
  snprintf(line, sizeof(line), "RSSI: %.0f dBm   LQI: %u", lastRxRssi, static_cast<unsigned>(lastRxLqi));
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(line, kMarginLeft, kRowInfo3, 1);

  snprintf(line, sizeof(line), "RX count: %lu", static_cast<unsigned long>(rxCounter));
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(line, kMarginLeft, kRowInfo4, 1);
}

void drawTransmitRows() {
  tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Burst TX:", kMarginLeft, kRowInfo1, 1);

  char line[64];
  snprintf(line, sizeof(line), "Every %lu ms", static_cast<unsigned long>(kBurstIntervalMs));
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(line, kMarginLeft, kRowInfo2, 1);

  snprintf(line, sizeof(line), "TX count: %lu", static_cast<unsigned long>(burstCounter));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(line, kMarginLeft, kRowInfo3, 2);

  String prefix = burstPrefix;
  if (prefix.length() > 32) {
    prefix = prefix.substring(0, 29) + "...";
  }
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(prefix, kMarginLeft, kRowInfo4, 1);
}

void drawSniffRows() {
  tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("OOK sniff:", kMarginLeft, kRowInfo1, 1);

  char line[64];
  snprintf(line, sizeof(line), "Edges: %lu   Bursts: %lu",
           static_cast<unsigned long>(gTotalEdges),
           static_cast<unsigned long>(gBurstCount));
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(line, kMarginLeft, kRowInfo2, 1);

  if (gLastBurstPulses > 0) {
    snprintf(line, sizeof(line), "Last: %u pulses", static_cast<unsigned>(gLastBurstPulses));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(line, kMarginLeft, kRowInfo3, 1);

    snprintf(line, sizeof(line), "S~%luus L~%luus",
             static_cast<unsigned long>(gLastBurstShortUs),
             static_cast<unsigned long>(gLastBurstLongUs));
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(line, kMarginLeft, kRowInfo4, 1);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Press the remote near antenna...", kMarginLeft, kRowInfo3, 1);
  }
}

void redrawAll() {
  prepareDisplayBus();

  if (needFullRedraw) {
    needFullRedraw = false;
    tft.fillRect(0, kHeaderHeight, tft.width(), tft.height() - kHeaderHeight - kFooterHeight, TFT_BLACK);
    drawFreqRow();
    drawModeRow();
    drawDivider();

    if (currentMode == RadioMode::SniffOok) {
      drawSniffRows();
    } else if (currentMode == RadioMode::Receive) {
      drawReceiveRows();
    } else {
      drawTransmitRows();
    }

    if (!radioReady) {
      drawFooter("Radio init failed");
    } else {
      drawFooter("USR=freq  ENC=mode");
    }
    return;
  }

  if (currentMode == RadioMode::SniffOok) {
    if (needSniffStatsRedraw) {
      needSniffStatsRedraw = false;
      drawSniffRows();
    }
    return;
  }

  if (currentMode == RadioMode::Receive) {
    drawReceiveRows();
  } else {
    drawTransmitRows();
  }
}

void printHelp() {
  Serial.println();
  Serial.println(F("CC1101 send/receive test commands:"));
  Serial.println(F("  help              - show this help"));
  Serial.println(F("  status            - show current settings"));
  Serial.println(F("  rx                - enter receive mode"));
  Serial.println(F("  tx                - send a test packet every second"));
  Serial.println(F("  sniff             - raw OOK pulse sniffer"));
  Serial.println(F("  send <text>       - send one packet immediately"));
  Serial.println(F("  freq 315|433|868  - switch frequency"));
  Serial.println(F("  prefix <text>     - change periodic TX message prefix"));
  Serial.println(F("Hardware controls:"));
  Serial.println(F("  USR key           - cycle frequency"));
  Serial.println(F("  ENC key           - cycle RX -> TX -> SNIFF"));
  Serial.println();
}

void printStatus() {
  Serial.println();
  Serial.print(F("[CC1101] Mode:        "));
  Serial.println(modeLabel(currentMode));
  Serial.print(F("[CC1101] Frequency:   "));
  Serial.print(currentFrequencyMHz(), 2);
  Serial.println(F(" MHz"));
  Serial.print(F("[CC1101] Modulation:  "));
  Serial.println(kUseOok ? F("OOK") : F("2-FSK"));
  Serial.print(F("[CC1101] Bit rate:    "));
  Serial.print(kBitRateKbps, 1);
  Serial.println(F(" kbps"));
  Serial.print(F("[CC1101] RX BW:       "));
  Serial.print(kRxBandwidthKHz, 1);
  Serial.println(F(" kHz"));
  Serial.print(F("[CC1101] Sync word:   0x"));
  Serial.print(kSyncWordHigh, HEX);
  Serial.print(F(" 0x"));
  Serial.println(kSyncWordLow, HEX);
  Serial.print(F("[CC1101] TX prefix:   "));
  Serial.println(burstPrefix);
}

bool applyRadioSettings() {
  prepareRadioBus();

  int state = radio.begin(currentFrequencyMHz());
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] radio.begin failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setFrequency(currentFrequencyMHz());
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setFrequency failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setOOK(kUseOok);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setOOK failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setBitRate(kBitRateKbps);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setBitRate failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setRxBandwidth(kRxBandwidthKHz);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setRxBandwidth failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setFrequencyDeviation(kFrequencyDeviationKHz);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setFrequencyDeviation failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setOutputPower(kOutputPowerDbm);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setOutputPower failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setSyncWord(kSyncWordHigh, kSyncWordLow);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] setSyncWord failed, code "));
    Serial.println(state);
    return false;
  }

  return true;
}

bool initRadio() {
  if (!setRfPathForFrequencyIndex(currentFreqIndex)) {
    Serial.println(F("[CC1101] Unsupported frequency index."));
    return false;
  }

  prepareRadioBus();
  radioSPI.begin(BOARD_LORA_SCK, BOARD_LORA_MISO, BOARD_LORA_MOSI);
  delay(20);

  Serial.print(F("[CC1101] Initializing at "));
  Serial.print(currentFrequencyMHz(), 2);
  Serial.println(F(" MHz ..."));

  return applyRadioSettings();
}

void leaveSniffMode() {
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO0));
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO2));

  prepareRadioBus();
  (void)radio.SPIsendCommand(kCcCmdSidle);
  (void)radio.SPIsendCommand(kCcCmdSfrx);
  (void)applyRadioSettings();
}

bool enterReceiveMode() {
  const bool wasSniff = (currentMode == RadioMode::SniffOok);
  currentMode = RadioMode::Receive;
  packetReceived = false;
  needFullRedraw = true;
  screenDirty = true;

  if (wasSniff) {
    leaveSniffMode();
  }

  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO0));
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO2));

  prepareRadioBus();
  radio.clearPacketSentAction();
  radio.clearPacketReceivedAction();
  (void)radio.finishTransmit();
  (void)radio.standby();

  radio.setPacketReceivedAction(onPacketReceived);
  const int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] startReceive failed, code "));
    Serial.println(state);
    return false;
  }

  Serial.println(F("[CC1101] Mode switched to RX."));
  return true;
}

void enterBurstTransmitMode() {
  const bool wasSniff = (currentMode == RadioMode::SniffOok);
  currentMode = RadioMode::BurstTransmit;
  lastBurstAtMs = 0;
  needFullRedraw = true;
  screenDirty = true;

  if (wasSniff) {
    leaveSniffMode();
  }

  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO0));
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO2));

  prepareRadioBus();
  radio.clearPacketReceivedAction();
  radio.clearPacketSentAction();
  (void)radio.finishReceive();
  (void)radio.standby();

  Serial.println(F("[CC1101] Mode switched to TX burst."));
}

bool enterSniffMode() {
  currentMode = RadioMode::SniffOok;
  needFullRedraw = true;
  screenDirty = true;

  gPulseHead = 0;
  gPulseTail = 0;
  gLastEdgeUs = micros();
  gIsrEdgeCount = 0;
  gTotalEdges = 0;
  gBurstCount = 0;
  gCurrentBurstPulses = 0;
  gCurrentBurstMinUs = 0;
  gCurrentBurstMaxUs = 0;
  gLastBurstPulses = 0;
  gLastBurstMinUs = 0;
  gLastBurstMaxUs = 0;
  gLastBurstShortUs = 0;
  gLastBurstLongUs = 0;
  gLastEdgeAtMs = 0;
  gLastSniffDrawMs = 0;
  gInBurst = false;

  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO0));
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO2));

  prepareRadioBus();
  radio.clearPacketReceivedAction();
  radio.clearPacketSentAction();
  (void)radio.finishReceive();
  (void)radio.finishTransmit();
  (void)radio.standby();

  int state = radio.setOOK(true);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] sniff: setOOK failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setRxBandwidth(kSniffRxBwKHz);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] sniff: setRxBandwidth failed, code "));
    Serial.println(state);
  }

  state = radio.setBitRate(kSniffBitRateKbps);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[CC1101] sniff: setBitRate failed, code "));
    Serial.println(state);
  }

  radio.SPIsetRegValue(kCcRegMdmCfg2, 0x00, 2, 0);
  radio.SPIsetRegValue(kCcRegIocfg0, kCcGdoSerialDataAsync);
  radio.SPIsetRegValue(kCcRegIocfg2, kCcGdoSerialDataAsync);
  radio.SPIsetRegValue(kCcRegPktCtrl0, kCcPktCtrl0AsyncSerial | 0x02);

  (void)radio.SPIsendCommand(kCcCmdSidle);
  (void)radio.SPIsendCommand(kCcCmdSfrx);
  (void)radio.SPIsendCommand(kCcCmdSrx);

  pinMode(BOARD_LORA_IO2, INPUT);
  attachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO2), onSniffEdge, CHANGE);

  Serial.print(F("[CC1101] Mode switched to OOK sniff @ "));
  Serial.print(currentFrequencyMHz(), 2);
  Serial.println(F(" MHz."));
  return true;
}

bool sendOnePacket(String payload, bool resumeRx) {
  prepareRadioBus();
  radio.clearPacketReceivedAction();
  radio.clearPacketSentAction();
  (void)radio.finishReceive();
  (void)radio.standby();

  Serial.print(F("[CC1101] TX -> "));
  Serial.println(payload);

  const int state = radio.transmit(payload);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[CC1101] TX success."));
    triggerLedEffect(LedEffect::TxFlash);
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    Serial.println(F("[CC1101] TX failed: packet too long."));
  } else {
    Serial.print(F("[CC1101] TX failed, code "));
    Serial.println(state);
  }

  if (resumeRx) {
    return enterReceiveMode();
  }

  return state == RADIOLIB_ERR_NONE;
}

bool reinitializeRadioForCurrentMode() {
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO0));
  detachInterrupt(digitalPinToInterrupt(BOARD_LORA_IO2));

  prepareRadioBus();
  radio.clearPacketReceivedAction();
  radio.clearPacketSentAction();
  (void)radio.finishReceive();
  (void)radio.finishTransmit();
  (void)radio.sleep();
  delay(10);

  if (!initRadio()) {
    radioReady = false;
    needFullRedraw = true;
    screenDirty = true;
    return false;
  }

  radioReady = true;

  if (currentMode == RadioMode::Receive) {
    return enterReceiveMode();
  }
  if (currentMode == RadioMode::SniffOok) {
    return enterSniffMode();
  }

  enterBurstTransmitMode();
  return true;
}

void handleReceivedPacket() {
  if (!packetReceived) {
    return;
  }
  packetReceived = false;

  prepareRadioBus();
  String payload;
  const int state = radio.readData(payload);
  if (state == RADIOLIB_ERR_NONE) {
    rxCounter++;
    lastRxPayload = payload;
    lastRxRssi = radio.getRSSI();
    lastRxLqi = radio.getLQI();
    hasLastRx = true;
    screenDirty = true;
    triggerLedEffect(LedEffect::RxFlash);

    Serial.println(F("[CC1101] RX packet received."));
    Serial.print(F("[CC1101] Data: "));
    Serial.println(payload);
    Serial.print(F("[CC1101] RSSI: "));
    Serial.print(lastRxRssi);
    Serial.println(F(" dBm"));
    Serial.print(F("[CC1101] LQI:  "));
    Serial.println(lastRxLqi);
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println(F("[CC1101] RX CRC mismatch."));
  } else {
    Serial.print(F("[CC1101] RX readData failed, code "));
    Serial.println(state);
  }

  if (currentMode == RadioMode::Receive) {
    prepareRadioBus();
    const int restartState = radio.startReceive();
    if (restartState != RADIOLIB_ERR_NONE) {
      Serial.print(F("[CC1101] Failed to resume RX, code "));
      Serial.println(restartState);
    }
  }
}

void drainSniffBuffer() {
  if (currentMode != RadioMode::SniffOok) {
    return;
  }

  noInterrupts();
  const uint32_t isrCount = gIsrEdgeCount;
  interrupts();
  gTotalEdges = isrCount;

  bool burstClosedThisPass = false;

  while (gPulseTail != gPulseHead) {
    PulseEvent ev;
    noInterrupts();
    ev.durationUs = gPulseRing[gPulseTail].durationUs;
    gPulseTail = (gPulseTail + 1U) % kPulseRingSize;
    interrupts();

    gLastEdgeAtMs = millis();

    if (ev.durationUs >= kBurstSilenceUs) {
      if (gInBurst && (gCurrentBurstPulses >= 4U)) {
        gLastBurstPulses = gCurrentBurstPulses;
        gLastBurstMinUs = gCurrentBurstMinUs;
        gLastBurstMaxUs = gCurrentBurstMaxUs;
        const uint32_t mid = (gLastBurstMinUs + gLastBurstMaxUs) / 2U;
        gLastBurstShortUs = (gLastBurstMinUs + mid) / 2U;
        gLastBurstLongUs = (gLastBurstMaxUs + mid) / 2U;
        gBurstCount++;
        burstClosedThisPass = true;

        Serial.print(F("[CC1101] Burst captured: "));
        Serial.print(gCurrentBurstPulses);
        Serial.print(F(" pulses, "));
        Serial.print(gCurrentBurstMinUs);
        Serial.print(F("us .. "));
        Serial.print(gCurrentBurstMaxUs);
        Serial.println(F("us"));
      }

      gInBurst = false;
      gCurrentBurstPulses = 0;
      gCurrentBurstMinUs = 0;
      gCurrentBurstMaxUs = 0;
      continue;
    }

    if ((ev.durationUs < kMinValidPulseUs) || (ev.durationUs > kMaxValidPulseUs)) {
      continue;
    }

    if (!gInBurst) {
      gInBurst = true;
      gCurrentBurstPulses = 0;
      gCurrentBurstMinUs = ev.durationUs;
      gCurrentBurstMaxUs = ev.durationUs;
    }

    gCurrentBurstPulses++;
    if (ev.durationUs < gCurrentBurstMinUs) {
      gCurrentBurstMinUs = ev.durationUs;
    }
    if (ev.durationUs > gCurrentBurstMaxUs) {
      gCurrentBurstMaxUs = ev.durationUs;
    }
  }

  if (gInBurst && ((millis() - gLastEdgeAtMs) > 50U)) {
    if (gCurrentBurstPulses >= 4U) {
      gLastBurstPulses = gCurrentBurstPulses;
      gLastBurstMinUs = gCurrentBurstMinUs;
      gLastBurstMaxUs = gCurrentBurstMaxUs;
      const uint32_t mid = (gLastBurstMinUs + gLastBurstMaxUs) / 2U;
      gLastBurstShortUs = (gLastBurstMinUs + mid) / 2U;
      gLastBurstLongUs = (gLastBurstMaxUs + mid) / 2U;
      gBurstCount++;
      burstClosedThisPass = true;

      Serial.print(F("[CC1101] Burst captured (timeout): "));
      Serial.print(gCurrentBurstPulses);
      Serial.println(F(" pulses"));
    }

    gInBurst = false;
    gCurrentBurstPulses = 0;
    gCurrentBurstMinUs = 0;
    gCurrentBurstMaxUs = 0;
  }

  const uint32_t now = millis();
  if (burstClosedThisPass) {
    gLastSniffDrawMs = now;
    needSniffStatsRedraw = true;
    screenDirty = true;
    triggerLedEffect(LedEffect::SniffFlash);
  } else if ((now - gLastSniffDrawMs) > kSniffScreenIntervalMs) {
    gLastSniffDrawMs = now;
    needSniffStatsRedraw = true;
    screenDirty = true;
  }
}

bool parseFrequencyToIndex(const String& input, uint8_t& outIndex) {
  String trimmed = input;
  trimmed.trim();

  if (trimmed.equals("315")) {
    outIndex = 0;
    return true;
  }
  if (trimmed.equals("433") || trimmed.equals("434")) {
    outIndex = 1;
    return true;
  }
  if (trimmed.equals("868")) {
    outIndex = 2;
    return true;
  }
  return false;
}

void handleCommand(String line) {
  line.trim();
  if (line.isEmpty()) {
    return;
  }

  if (line.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }

  if (line.equalsIgnoreCase("status")) {
    printStatus();
    return;
  }

  if (line.equalsIgnoreCase("rx")) {
    (void)enterReceiveMode();
    return;
  }

  if (line.equalsIgnoreCase("tx")) {
    enterBurstTransmitMode();
    return;
  }

  if (line.equalsIgnoreCase("sniff")) {
    (void)enterSniffMode();
    return;
  }

  if (line.startsWith("send ")) {
    String payload = line.substring(5);
    payload.trim();
    if (payload.isEmpty()) {
      Serial.println(F("[CC1101] Empty payload ignored."));
      return;
    }

    const bool resumeRx = (currentMode == RadioMode::Receive);
    (void)sendOnePacket(payload, resumeRx);
    return;
  }

  if (line.startsWith("prefix ")) {
    String prefix = line.substring(7);
    prefix.trim();
    if (prefix.isEmpty()) {
      Serial.println(F("[CC1101] Prefix cannot be empty."));
      return;
    }

    burstPrefix = prefix;
    screenDirty = true;
    Serial.print(F("[CC1101] TX prefix updated to: "));
    Serial.println(burstPrefix);
    return;
  }

  if (line.startsWith("freq ")) {
    uint8_t newIndex = 0;
    if (!parseFrequencyToIndex(line.substring(5), newIndex)) {
      Serial.println(F("[CC1101] Unsupported frequency. Use 315, 433 or 868."));
      return;
    }

    currentFreqIndex = newIndex;
    if (reinitializeRadioForCurrentMode()) {
      Serial.print(F("[CC1101] Frequency switched to "));
      Serial.print(currentFrequencyMHz(), 2);
      Serial.println(F(" MHz."));
    }
    return;
  }

  Serial.print(F("[CC1101] Unknown command: "));
  Serial.println(line);
  printHelp();
}

void pollSerialCommands() {
  if (!Serial.available()) {
    return;
  }

  String line = Serial.readStringUntil('\n');
  handleCommand(line);
}

void handleBurstTransmit() {
  if (currentMode != RadioMode::BurstTransmit) {
    return;
  }

  const unsigned long now = millis();
  if ((lastBurstAtMs != 0U) && ((now - lastBurstAtMs) < kBurstIntervalMs)) {
    return;
  }

  lastBurstAtMs = now;
  const String payload = burstPrefix + " #" + String(burstCounter++);
  (void)sendOnePacket(payload, false);
  screenDirty = true;
}

void cycleFrequency() {
  currentFreqIndex = (currentFreqIndex + 1U) % kFreqChoiceCount;
  Serial.print(F("[CC1101] USR key -> freq "));
  Serial.println(currentFrequencyLabel());
  (void)reinitializeRadioForCurrentMode();
}

void toggleMode() {
  switch (currentMode) {
    case RadioMode::Receive:
      enterBurstTransmitMode();
      break;
    case RadioMode::BurstTransmit:
      (void)enterSniffMode();
      break;
    case RadioMode::SniffOok:
    default:
      (void)enterReceiveMode();
      break;
  }
}

void pollButtons() {
  static bool lastUsr = false;
  static bool lastEnc = false;

  const bool usr = (digitalRead(BOARD_USER_KEY) == LOW);
  const bool enc = (digitalRead(ENCODER_KEY) == LOW);

  if (usr && !lastUsr) {
    cycleFrequency();
  }
  if (enc && !lastEnc) {
    toggleMode();
  }

  lastUsr = usr;
  lastEnc = enc;
}

void initBoardPower() {
  pinMode(BOARD_PWR_EN, OUTPUT);
  digitalWrite(BOARD_PWR_EN, HIGH);

  pinMode(DISPLAY_BL, OUTPUT);
  digitalWrite(DISPLAY_BL, HIGH);
}

void initLeds() {
  strip.begin();
  strip.setBrightness(255);
  setStripColor(0, 0, 0);
}

void initDisplay() {
  prepareDisplayBus();
  tft.init();
  tft.setRotation(kRotation);
  tft.fillScreen(TFT_BLACK);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(400);

  Serial.println();
  Serial.println(F("T-Embed CC1101 send/receive test"));

  initSharedSpiPins();
  initBoardPower();
  initDisplay();

  pinMode(BOARD_USER_KEY, INPUT_PULLUP);
  pinMode(ENCODER_KEY, INPUT_PULLUP);

  initLeds();

  prepareDisplayBus();
  drawHeader();
  drawFooter("Initializing radio...");

  radioReady = initRadio();
  if (!radioReady) {
    Serial.println(F("[CC1101] Radio init failed."));
  } else if (!enterReceiveMode()) {
    Serial.println(F("[CC1101] Failed to enter RX mode."));
    radioReady = false;
  }

  redrawAll();
  printStatus();
  printHelp();
}

void loop() {
  pollSerialCommands();
  pollButtons();

  if (radioReady) {
    handleReceivedPacket();
    handleBurstTransmit();
    drainSniffBuffer();
  }

  checkLedTimeout();
  updateLeds();

  if (screenDirty) {
    screenDirty = false;
    redrawAll();
  }

  delay(2);
}
