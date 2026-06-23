#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>

#include "utilities.h"

extern SPIClass spi;

namespace {

struct ChannelChoice {
  uint8_t channel;
  const char* label;
};

struct DataRateChoice {
  uint16_t kbps;
  const char* label;
};

struct PowerChoice {
  int8_t dbm;
  const char* label;
};

constexpr ChannelChoice kChannelChoices[] = {
  {0, "CH 000"},
  {40, "CH 040"},
  {76, "CH 076"},
  {125, "CH 125"},
};
constexpr uint8_t kChannelChoiceCount = sizeof(kChannelChoices) / sizeof(kChannelChoices[0]);

constexpr DataRateChoice kDataRateChoices[] = {
  {250, "250K"},
  {1000, "1M"},
  {2000, "2M"},
};
constexpr uint8_t kDataRateChoiceCount = sizeof(kDataRateChoices) / sizeof(kDataRateChoices[0]);

constexpr PowerChoice kPowerChoices[] = {
  {-18, "-18"},
  {-12, "-12"},
  {-6, "-6"},
  {0, "0"},
};
constexpr uint8_t kPowerChoiceCount = sizeof(kPowerChoices) / sizeof(kPowerChoices[0]);

constexpr uint8_t kAddressWidth = 5;
constexpr uint8_t kPipeAddress[kAddressWidth] = {0x01, 0x23, 0x45, 0x67, 0x89};
constexpr uint32_t kBurstIntervalMs = 1000;
constexpr uint32_t kTxTimeoutMs = 100;
constexpr char kDefaultTxPrefix[] = "T-Embed nRF24";

constexpr uint8_t kRotation = 3;
constexpr int16_t kHeaderHeight = 24;
constexpr int16_t kFooterHeight = 18;
constexpr int16_t kMarginLeft = 8;
constexpr int16_t kRowParam = 30;
constexpr int16_t kRowMode = 64;
constexpr int16_t kRowDivider = 86;
constexpr int16_t kRowInfo1 = 92;
constexpr int16_t kRowInfo2 = 108;
constexpr int16_t kRowInfo3 = 124;
constexpr int16_t kRowInfo4 = 140;

constexpr uint8_t kLedBrightness = 10;
constexpr uint32_t kLedFlashMs = 200;

enum class RadioMode : uint8_t {
  Receive,
  BurstTransmit,
};

enum class LedEffect : uint8_t {
  None,
  RxFlash,
  TxFlash,
};

nRF24 radio = new Module(BOARD_NRF24_CS, BOARD_NRF24_IRQ, BOARD_NRF24_CE, RADIOLIB_NC, spi);
TFT_eSPI tft;
Adafruit_NeoPixel strip(WS2812_NUM_LEDS, WS2812_DATA_PIN, NEO_GRB + NEO_KHZ800);

RadioMode currentMode = RadioMode::Receive;
uint8_t currentChannelIndex = 2;
uint8_t currentChannelValue = 76;
uint8_t currentDataRateIndex = 1;
uint8_t currentPowerIndex = 3;
bool autoAckEnabled = true;

unsigned long lastBurstAtMs = 0;
uint32_t burstCounter = 0;
uint32_t rxCounter = 0;
String burstPrefix = kDefaultTxPrefix;

String lastRxPayload;
size_t lastRxLength = 0;
bool hasLastRx = false;
bool radioReady = false;
bool screenDirty = true;
bool needFullRedraw = true;

LedEffect gLedEffect = LedEffect::None;
uint32_t gLedEffectUntilMs = 0;
bool gLedDirty = true;

inline uint8_t currentChannel() {
  return currentChannelValue;
}

inline const char* currentChannelLabel() {
  static char label[12];
  snprintf(label, sizeof(label), "CH %03u", currentChannelValue);
  return label;
}

inline uint16_t currentDataRateKbps() {
  return kDataRateChoices[currentDataRateIndex].kbps;
}

inline const char* currentDataRateLabel() {
  return kDataRateChoices[currentDataRateIndex].label;
}

inline int8_t currentOutputPowerDbm() {
  return kPowerChoices[currentPowerIndex].dbm;
}

inline const char* currentOutputPowerLabel() {
  return kPowerChoices[currentPowerIndex].label;
}

inline float currentFrequencyMHz() {
  return 2400.0f + static_cast<float>(currentChannel());
}

const char* modeLabel(RadioMode mode) {
  switch (mode) {
    case RadioMode::Receive:
      return "RX";
    case RadioMode::BurstTransmit:
      return "TX-BURST";
    default:
      return "?";
  }
}

void deselectSharedSpiDevices() {
  digitalWrite(DISPLAY_CS, HIGH);
  digitalWrite(BOARD_SD_CS, HIGH);
  digitalWrite(BOARD_LORA_CS, HIGH);
  digitalWrite(BOARD_NRF24_CS, HIGH);
}

void initSharedSpiPins() {
  pinMode(DISPLAY_CS, OUTPUT);
  pinMode(BOARD_SD_CS, OUTPUT);
  pinMode(BOARD_LORA_CS, OUTPUT);
  pinMode(BOARD_NRF24_CS, OUTPUT);
  deselectSharedSpiDevices();
}

void prepareDisplayBus() {
  deselectSharedSpiDevices();
}

void prepareRadioBus() {
  deselectSharedSpiDevices();
}

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
  tft.drawString("nRF24 Send / Recv", kMarginLeft, 6, 2);
}

void drawFooter(const char* msg) {
  const int16_t y = tft.height() - kFooterHeight;
  tft.fillRect(0, y, tft.width(), kFooterHeight, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.drawString(msg, kMarginLeft, y + 3, 1);
}

void drawChannelRow() {
  tft.fillRect(0, kRowParam, tft.width(), 28, TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("CHAN:", kMarginLeft, kRowParam + 8, 1);

  int8_t presetIndex = -1;
  for (uint8_t i = 0; i < kChannelChoiceCount; ++i) {
    if (kChannelChoices[i].channel == currentChannelValue) {
      presetIndex = static_cast<int8_t>(i);
      break;
    }
  }

  uint16_t color = TFT_WHITE;
  if (presetIndex == 0) {
    color = TFT_GREEN;
  } else if (presetIndex == 1) {
    color = TFT_YELLOW;
  } else if (presetIndex == 2) {
    color = TFT_CYAN;
  } else if (presetIndex == 3) {
    color = TFT_ORANGE;
  }

  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(currentChannelLabel(), kMarginLeft + 50, kRowParam, 4);
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
    default:
      break;
  }

  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(label, kMarginLeft + 50, kRowMode, 2);
}

void drawDivider() {
  tft.drawFastHLine(0, kRowDivider, tft.width(), TFT_DARKGREY);
}

void drawConfigSummaryLine(int16_t y) {
  char line[64];
  snprintf(line, sizeof(line), "CH%03u %.0fMHz %s %sdBm %s",
           currentChannel(),
           currentFrequencyMHz(),
           currentDataRateLabel(),
           currentOutputPowerLabel(),
           autoAckEnabled ? "ACK" : "NOACK");
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(line, kMarginLeft, y, 1);
}

void drawReceiveRows() {
  tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Last RX:", kMarginLeft, kRowInfo1, 1);

  if (!hasLastRx) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("(none)", kMarginLeft + 56, kRowInfo1, 1);
    tft.drawString("Waiting for packets...", kMarginLeft, kRowInfo2, 1);

    char idleMeta[32];
    snprintf(idleMeta, sizeof(idleMeta), "RX count: %lu", static_cast<unsigned long>(rxCounter));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(idleMeta, kMarginLeft, kRowInfo3, 1);
  } else {
    String payload = lastRxPayload;
    if (payload.length() > 38) {
      payload = payload.substring(0, 35) + "...";
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(payload, kMarginLeft, kRowInfo2, 1);

    char meta[40];
    snprintf(meta, sizeof(meta), "LEN:%u  RX#%lu",
             static_cast<unsigned>(lastRxLength),
             static_cast<unsigned long>(rxCounter));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(meta, kMarginLeft, kRowInfo3, 1);
  }

  drawConfigSummaryLine(kRowInfo4);
}

void drawTransmitRows() {
  tft.fillRect(0, kRowInfo1, tft.width(), tft.height() - kFooterHeight - kRowInfo1, TFT_BLACK);

  char line[48];
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  snprintf(line, sizeof(line), "Burst TX every %lu ms", static_cast<unsigned long>(kBurstIntervalMs));
  tft.drawString(line, kMarginLeft, kRowInfo1, 1);

  snprintf(line, sizeof(line), "TX count: %lu", static_cast<unsigned long>(burstCounter));
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(line, kMarginLeft, kRowInfo2, 1);

  String prefix = burstPrefix;
  if (prefix.length() > 28) {
    prefix = prefix.substring(0, 25) + "...";
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Prefix: " + prefix, kMarginLeft, kRowInfo3, 1);
  drawConfigSummaryLine(kRowInfo4);
}

void redrawAll() {
  prepareDisplayBus();

  if (needFullRedraw) {
    needFullRedraw = false;
    tft.fillRect(0, kHeaderHeight, tft.width(), tft.height() - kHeaderHeight - kFooterHeight, TFT_BLACK);
    drawChannelRow();
    drawModeRow();
    drawDivider();

    if (currentMode == RadioMode::Receive) {
      drawReceiveRows();
    } else {
      drawTransmitRows();
    }

    if (!radioReady) {
      drawFooter("Radio init failed");
    } else {
      drawFooter("USR=chan  ENC=mode");
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
  Serial.println(F("nRF24 send/receive test commands:"));
  Serial.println(F("  help               - show this help"));
  Serial.println(F("  status             - show current settings"));
  Serial.println(F("  rx                 - enter receive mode"));
  Serial.println(F("  tx                 - send a test packet every second"));
  Serial.println(F("  send <text>        - send one packet immediately"));
  Serial.println(F("  ch <0-125>         - switch RF channel"));
  Serial.println(F("  rate 250|1000|2000 - switch data rate (kbps)"));
  Serial.println(F("  power -18|-12|-6|0 - switch output power"));
  Serial.println(F("  ack on|off         - set auto ACK"));
  Serial.println(F("  prefix <text>      - change periodic TX message prefix"));
  Serial.println(F("Hardware controls:"));
  Serial.println(F("  USR key            - cycle channel preset"));
  Serial.println(F("  ENC key            - cycle RX <-> TX"));
  Serial.println();
}

void printStatus() {
  Serial.println();
  Serial.print(F("[nRF24] Mode:         "));
  Serial.println(modeLabel(currentMode));
  Serial.print(F("[nRF24] Channel:      "));
  Serial.println(currentChannel());
  Serial.print(F("[nRF24] Frequency:    "));
  Serial.print(currentFrequencyMHz(), 0);
  Serial.println(F(" MHz"));
  Serial.print(F("[nRF24] Data rate:    "));
  Serial.print(currentDataRateKbps());
  Serial.println(F(" kbps"));
  Serial.print(F("[nRF24] Output power: "));
  Serial.print(currentOutputPowerDbm());
  Serial.println(F(" dBm"));
  Serial.print(F("[nRF24] Auto ACK:     "));
  Serial.println(autoAckEnabled ? F("ON") : F("OFF"));
  Serial.print(F("[nRF24] TX prefix:    "));
  Serial.println(burstPrefix);
}

void configureNrf24SpiProtocol() {
  // nRF24 returns STATUS in parallel with the command byte, so there is no
  // extra status byte between the command and payload data stream.
  radio.getMod()->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  radio.getMod()->spiConfig.statusPos = 0;
}

bool applyRadioSettings() {
  prepareRadioBus();
  configureNrf24SpiProtocol();

  int state = radio.begin(static_cast<int16_t>(currentFrequencyMHz()),
                          static_cast<int16_t>(currentDataRateKbps()),
                          currentOutputPowerDbm(),
                          kAddressWidth);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] radio.begin failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setAddressWidth(kAddressWidth);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setAddressWidth failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setFrequency(currentFrequencyMHz());
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setFrequency failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setBitRate(currentDataRateKbps());
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setBitRate failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setOutputPower(currentOutputPowerDbm());
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setOutputPower failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setAutoAck(autoAckEnabled);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setAutoAck failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setReceivePipe(0, kPipeAddress);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setReceivePipe failed, code "));
    Serial.println(state);
    return false;
  }

  state = radio.setTransmitPipe(kPipeAddress);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] setTransmitPipe failed, code "));
    Serial.println(state);
    return false;
  }

  return true;
}

bool initRadio() {
  prepareRadioBus();
  delay(20);

  Serial.print(F("[nRF24] Initializing at CH "));
  Serial.print(currentChannel());
  Serial.print(F(" / "));
  Serial.print(currentFrequencyMHz(), 0);
  Serial.println(F(" MHz ..."));

  return applyRadioSettings();
}

bool enterReceiveMode() {
  currentMode = RadioMode::Receive;
  needFullRedraw = true;
  screenDirty = true;

  prepareRadioBus();
  (void)radio.finishTransmit();
  (void)radio.finishReceive();
  (void)radio.standby();
  (void)radio.setReceivePipe(0, kPipeAddress);

  const int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[nRF24] startReceive failed, code "));
    Serial.println(state);
    return false;
  }

  Serial.println(F("[nRF24] Mode switched to RX."));
  return true;
}

void enterBurstTransmitMode() {
  currentMode = RadioMode::BurstTransmit;
  lastBurstAtMs = 0;
  needFullRedraw = true;
  screenDirty = true;

  prepareRadioBus();
  (void)radio.finishReceive();
  (void)radio.finishTransmit();
  (void)radio.standby();
  (void)radio.setTransmitPipe(kPipeAddress);

  Serial.println(F("[nRF24] Mode switched to TX burst."));
}

bool transmitBlocking(String payload) {
  prepareRadioBus();
  (void)radio.finishReceive();
  (void)radio.finishTransmit();
  (void)radio.standby();
  (void)radio.setTransmitPipe(kPipeAddress);

  Serial.print(F("[nRF24] TX -> "));
  Serial.println(payload);

  int state = radio.startTransmit(reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), 0);
  if (state != RADIOLIB_ERR_NONE) {
    if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
      Serial.println(F("[nRF24] TX failed: packet too long."));
    } else {
      Serial.print(F("[nRF24] startTransmit failed, code "));
      Serial.println(state);
    }
    (void)radio.finishTransmit();
    return false;
  }

  const uint32_t startedAtMs = millis();
  while ((millis() - startedAtMs) < kTxTimeoutMs) {
    const int txDone = radio.getStatus(RADIOLIB_NRF24_TX_DS);
    const int txFailed = radio.getStatus(RADIOLIB_NRF24_MAX_RT);

    if (txFailed > 0) {
      Serial.println(F("[nRF24] TX failed: ACK not received."));
      (void)radio.finishTransmit();
      return false;
    }

    if (txDone > 0) {
      Serial.println(F("[nRF24] TX success."));
      (void)radio.finishTransmit();
      triggerLedEffect(LedEffect::TxFlash);
      return true;
    }

    delay(1);
  }

  Serial.println(F("[nRF24] TX failed: timeout."));
  (void)radio.finishTransmit();
  return false;
}

bool sendOnePacket(String payload, bool resumeRx) {
  const bool ok = transmitBlocking(payload);
  if (resumeRx) {
    return enterReceiveMode() && ok;
  }
  return ok;
}

bool reinitializeRadioForCurrentMode() {
  prepareRadioBus();
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

  enterBurstTransmitMode();
  return true;
}

void handleReceivedPacket() {
  if (currentMode != RadioMode::Receive) {
    return;
  }

  prepareRadioBus();
  const int rxReady = radio.getStatus(RADIOLIB_NRF24_RX_DR);
  if (rxReady <= 0) {
    return;
  }

  lastRxLength = radio.getPacketLength();
  if (lastRxLength > RADIOLIB_NRF24_MAX_PACKET_LENGTH) {
    Serial.print(F("[nRF24] Invalid RX payload length: "));
    Serial.println(static_cast<unsigned>(lastRxLength));
    (void)radio.finishReceive();
    if (currentMode == RadioMode::Receive) {
      prepareRadioBus();
      (void)radio.startReceive();
    }
    return;
  }

  uint8_t buffer[RADIOLIB_NRF24_MAX_PACKET_LENGTH + 1] = {0};
  const int state = radio.readData(buffer, 0);
  if (state == RADIOLIB_ERR_NONE) {
    String textPayload;
    if (lastRxLength == 0U) {
      textPayload = "(empty)";
    } else {
      textPayload.reserve(lastRxLength);
      for (size_t i = 0; i < lastRxLength; ++i) {
        const char c = static_cast<char>(buffer[i]);
        if ((c == '\r') || (c == '\n') || (c == '\t')) {
          textPayload += ' ';
        } else {
          textPayload += c;
        }
      }
    }

    rxCounter++;
    lastRxPayload = textPayload;
    hasLastRx = true;
    screenDirty = true;
    triggerLedEffect(LedEffect::RxFlash);

    Serial.println(F("[nRF24] RX packet received."));
    Serial.print(F("[nRF24] Data: "));
    Serial.println(textPayload);
    Serial.print(F("[nRF24] Length: "));
    Serial.println(static_cast<unsigned>(lastRxLength));
  } else {
    Serial.print(F("[nRF24] RX readData failed, code "));
    Serial.println(state);
  }

  if (currentMode == RadioMode::Receive) {
    prepareRadioBus();
    const int restartState = radio.startReceive();
    if (restartState != RADIOLIB_ERR_NONE) {
      Serial.print(F("[nRF24] Failed to resume RX, code "));
      Serial.println(restartState);
    }
  }
}

bool parseChannelValue(const String& input, uint8_t& outChannel) {
  String trimmed = input;
  trimmed.trim();
  if (trimmed.isEmpty()) {
    return false;
  }

  const int value = trimmed.toInt();
  if ((value < 0) || (value > 125)) {
    return false;
  }

  outChannel = static_cast<uint8_t>(value);
  return true;
}

bool parseDataRateToIndex(const String& input, uint8_t& outIndex) {
  const int value = input.toInt();
  for (uint8_t i = 0; i < kDataRateChoiceCount; ++i) {
    if (kDataRateChoices[i].kbps == value) {
      outIndex = i;
      return true;
    }
  }
  return false;
}

bool parsePowerToIndex(const String& input, uint8_t& outIndex) {
  const int value = input.toInt();
  for (uint8_t i = 0; i < kPowerChoiceCount; ++i) {
    if (kPowerChoices[i].dbm == value) {
      outIndex = i;
      return true;
    }
  }
  return false;
}

bool parseAckSetting(const String& input, bool& outAckEnabled) {
  String trimmed = input;
  trimmed.trim();
  trimmed.toLowerCase();
  if (trimmed == "on" || trimmed == "1" || trimmed == "true") {
    outAckEnabled = true;
    return true;
  }
  if (trimmed == "off" || trimmed == "0" || trimmed == "false") {
    outAckEnabled = false;
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

  if (line.startsWith("send ")) {
    String payload = line.substring(5);
    payload.trim();
    if (payload.isEmpty()) {
      Serial.println(F("[nRF24] Empty payload ignored."));
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
      Serial.println(F("[nRF24] Prefix cannot be empty."));
      return;
    }

    burstPrefix = prefix;
    screenDirty = true;
    Serial.print(F("[nRF24] TX prefix updated to: "));
    Serial.println(burstPrefix);
    return;
  }

  if (line.startsWith("ch ")) {
    uint8_t newChannel = 0;
    if (!parseChannelValue(line.substring(3), newChannel)) {
      Serial.println(F("[nRF24] Unsupported channel. Use a value from 0 to 125."));
      return;
    }

    currentChannelValue = newChannel;
    for (uint8_t i = 0; i < kChannelChoiceCount; ++i) {
      if (kChannelChoices[i].channel == currentChannelValue) {
        currentChannelIndex = i;
        break;
      }
    }
    if (reinitializeRadioForCurrentMode()) {
      Serial.print(F("[nRF24] Channel switched to "));
      Serial.println(currentChannel());
    }
    return;
  }

  if (line.startsWith("rate ")) {
    uint8_t newIndex = 0;
    if (!parseDataRateToIndex(line.substring(5), newIndex)) {
      Serial.println(F("[nRF24] Unsupported rate. Use 250, 1000 or 2000."));
      return;
    }

    currentDataRateIndex = newIndex;
    if (reinitializeRadioForCurrentMode()) {
      Serial.print(F("[nRF24] Data rate switched to "));
      Serial.print(currentDataRateKbps());
      Serial.println(F(" kbps."));
    }
    return;
  }

  if (line.startsWith("power ")) {
    uint8_t newIndex = 0;
    if (!parsePowerToIndex(line.substring(6), newIndex)) {
      Serial.println(F("[nRF24] Unsupported power. Use -18, -12, -6 or 0."));
      return;
    }

    currentPowerIndex = newIndex;
    if (reinitializeRadioForCurrentMode()) {
      Serial.print(F("[nRF24] Power switched to "));
      Serial.print(currentOutputPowerDbm());
      Serial.println(F(" dBm."));
    }
    return;
  }

  if (line.startsWith("ack ")) {
    bool newAck = autoAckEnabled;
    if (!parseAckSetting(line.substring(4), newAck)) {
      Serial.println(F("[nRF24] Unsupported ACK value. Use on or off."));
      return;
    }

    autoAckEnabled = newAck;
    if (reinitializeRadioForCurrentMode()) {
      Serial.print(F("[nRF24] Auto ACK "));
      Serial.println(autoAckEnabled ? F("enabled.") : F("disabled."));
    }
    return;
  }

  Serial.print(F("[nRF24] Unknown command: "));
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

void cycleChannel() {
  currentChannelIndex = (currentChannelIndex + 1U) % kChannelChoiceCount;
  currentChannelValue = kChannelChoices[currentChannelIndex].channel;
  Serial.print(F("[nRF24] USR key -> channel "));
  Serial.println(currentChannel());
  (void)reinitializeRadioForCurrentMode();
}

void toggleMode() {
  switch (currentMode) {
    case RadioMode::Receive:
      enterBurstTransmitMode();
      break;
    case RadioMode::BurstTransmit:
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
    cycleChannel();
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
  Serial.println(F("T-Embed nRF24 send/receive test"));

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
    Serial.println(F("[nRF24] Radio init failed."));
  } else if (!enterReceiveMode()) {
    Serial.println(F("[nRF24] Failed to enter RX mode."));
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
  }

  checkLedTimeout();
  updateLeds();

  if (screenDirty) {
    screenDirty = false;
    redrawAll();
  }

  delay(2);
}
