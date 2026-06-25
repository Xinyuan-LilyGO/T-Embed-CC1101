#pragma once

namespace page_battery {

namespace {
constexpr uint32_t kPollIntervalMs = 1000;
constexpr uint32_t kUiStateDebounceMs = 2500;
constexpr uint32_t kExternalPowerHoldMs = 1800;
constexpr uint32_t kChargingStateHoldMs = 1800;
constexpr uint32_t kChargeTopOffRetryMs = 60000;
constexpr uint32_t kChargeRecoveryRetryIntervalMs = 1500;
constexpr uint32_t kGaugeStartupSettleMs = 120;
constexpr uint32_t kGaugeInitRetryDelayMs = 120;
constexpr uint32_t kShutdownHoldMs = 2000;
constexpr uint32_t kTransientDetailMs = 2500;
constexpr uint32_t kUiFrameIntervalMs = 80;
constexpr uint8_t kGaugeInitAttempts = 3;

constexpr uint16_t kBatteryCapacityMah = 1300;
constexpr uint16_t kChargeTargetVoltageMv = 4208;
constexpr uint16_t kPrechargeCurrentMa = 128;
constexpr uint16_t kFastChargeCurrentMa = 512;
constexpr uint16_t kTerminationCurrentMa = 128;
constexpr uint16_t kRechargeThresholdOffsetMv = 100;
constexpr uint16_t kSysPowerDownVoltageMv = 3300;
constexpr uint16_t kInputCurrentSdpMa = 500;
constexpr uint16_t kInputCurrentAdapterMa = 1000;
constexpr uint16_t kChargeDoneSocThreshold = 100;
constexpr uint16_t kChargeTopOffRestartMarginMv = 32;
constexpr uint16_t kBqChargeTerminationVoltageMv = 100;
constexpr uint16_t kBqChargeDetectThresholdMa = 75;
constexpr uint16_t kBqQuitCurrentMa = 40;
constexpr uint16_t kVbusPresentThresholdMv = 3900;
constexpr int16_t kChargeCurrentIntoBatteryThresholdMa = 30;
constexpr int16_t kDischargeCurrentThresholdMa = -30;
constexpr int16_t kRecoveryDischargeCurrentMa = -50;

constexpr int16_t kLeftX = 8;
constexpr int16_t kLeftValueX = 62;
constexpr int16_t kRightX = 162;
constexpr int16_t kRightValueX = 222;
constexpr int16_t kRow0 = 80;
constexpr int16_t kRowGap = 14;
constexpr int16_t kStateX = 8;
constexpr int16_t kStateY = 30;
constexpr int16_t kDetailX = 8;
constexpr int16_t kDetailY = 64;

enum class UiState : uint8_t {
    Init = 0,
    Charging,
    ChargeDone,
    Discharging,
    Idle,
    BqConfigWarn,
    Error,
};

enum class FocusItem : uint8_t {
    Metrics = 0,
    Back,
    kCount,
};

struct UserButtonState {
    bool pressed = false;
    bool longPressHandled = false;
    uint32_t lastChangeMs = 0;
    uint32_t pressedAtMs = 0;
};

struct BatteryMetrics {
    uint16_t soc = 0;
    uint16_t bqVoltageMv = 0;
    int16_t ibatMa = 0;
    uint16_t remainMah = 0;
    uint16_t fullMah = 0;
    uint16_t designMah = 0;
    uint16_t soh = 0;
    int16_t tempDeciC = 0;
    uint16_t tteMin = 0;
    uint16_t ttfMin = 0;
    uint16_t pmuBattVoltageMv = 0;
    uint16_t vbusMv = 0;
    uint16_t vsysMv = 0;
    uint16_t chargeCurrentMa = 0;
    uint8_t busStatus = 0;
    uint8_t chargeStatus = 0;
    bool chargeEnabled = false;
    bool powerGood = false;
    bool hizMode = false;
    bool bqFullChargeDetected = false;
    bool isDischarging = false;
    bool vbusPresent = false;
};

BQ27220 gauge;
XPowersPPM pmu;
UserButtonState userButton;
BatteryMetrics metrics;
BatteryMetrics lastDrawnMetrics;
UiState uiState = UiState::Init;
UiState pendingUiState = UiState::Init;
FocusItem focus = FocusItem::Metrics;
bool hasMetrics = false;
bool gaugeReady = false;
bool pmuReady = false;
bool bqConfigWarning = false;
bool screenDirty = true;
bool frameDrawn = false;
String bqConfigDetail = "BQ cfg pending";
String transientDetail;
String lastDrawnDetail;
UiState lastDrawnUiState = UiState::Init;
unsigned long detailExpiresAtMs = 0;
unsigned long lastPollAtMs = 0;
unsigned long pendingUiStateSinceMs = 0;
unsigned long lastChargeTopOffAttemptMs = 0;
unsigned long lastChargingEvidenceAtMs = 0;
unsigned long lastExternalPowerSeenAtMs = 0;
unsigned long lastChargeRecoveryMs = 0;
uint32_t lastDrawMs = 0;
int16_t appliedInputCurrentMa = -1;
uint8_t latchedInputBusStatus = static_cast<uint8_t>(XPowersPPM::BUS_STATE_NOINPUT);
bool chargeRecoveryUsbWasPresent = false;
int32_t encSnapshot = 0;
uint8_t previewSoc = 0;
bool previewSocValid = false;
uint8_t previewChargeStatus = static_cast<uint8_t>(XPowersPPM::CHARGE_STATE_UNKOWN);
bool previewChargeValid = false;
bool previewVbusPresent = false;
uint32_t lastPreviewRefreshMs = 0;

BQ27220DMData makeDmU16Entry(const uint16_t address, const uint16_t value)
{
    BQ27220DMData entry = {};
    entry.type = BQ27220DMTypeU16;
    entry.address = address;
    entry.value.u16 = value;
    return entry;
}

BQ27220DMData makeDmEndEntry()
{
    BQ27220DMData entry = {};
    entry.type = BQ27220DMTypeEnd;
    return entry;
}

uint16_t readGaugeControlWord(const uint16_t subCommand)
{
    gauge.controlSubCmd(subCommand);
    delayMicroseconds(1000);
    return gauge.readRegU16(CommandMACData);
}

int16_t gaugeTemperatureDeciC()
{
    return static_cast<int16_t>(gauge.getTemperature()) - 2731;
}

uint16_t readTerminationCurrentMa()
{
    const int reg05 = pmu.readRegister(POWERS_PPM_REG_05H);
    if (reg05 < 0) {
        return 0;
    }
    return 64U + static_cast<uint16_t>(reg05 & 0x0F) * 64U;
}

uint16_t readRechargeThresholdOffsetMv()
{
    const int reg06 = pmu.readRegister(POWERS_PPM_REG_06H);
    if (reg06 < 0) {
        return 0;
    }
    return (reg06 & 0x01) ? 200U : 100U;
}

uint16_t readPmuChargeCurrentMa(const uint8_t chargeStatus)
{
    const auto status = static_cast<XPowersPPM::ChargeStatus>(chargeStatus);
    if (status == XPowersPPM::CHARGE_STATE_NO_CHARGE ||
        status == XPowersPPM::CHARGE_STATE_UNKOWN) {
        return 0;
    }
    return pmu.getChargeCurrent();
}

bool isUsableInputBusStatus(const uint8_t status)
{
    const auto bus = static_cast<XPowersPPM::BusStatus>(status);
    return bus != XPowersPPM::BUS_STATE_NOINPUT &&
           bus != XPowersPPM::BUS_STATE_OTG;
}

bool hasExternalPower(const BatteryMetrics& sample)
{
    return sample.vbusPresent &&
           static_cast<XPowersPPM::BusStatus>(sample.busStatus) != XPowersPPM::BUS_STATE_OTG;
}

bool hasExternalPower()
{
    return hasExternalPower(metrics);
}

bool hasChargingEvidence(const BatteryMetrics& sample)
{
    const auto chargeState = static_cast<XPowersPPM::ChargeStatus>(sample.chargeStatus);
    return chargeState == XPowersPPM::CHARGE_STATE_FAST_CHARGE ||
           chargeState == XPowersPPM::CHARGE_STATE_PRE_CHARGE ||
           sample.chargeCurrentMa > kChargeCurrentIntoBatteryThresholdMa;
}

uint16_t effectiveBatteryVoltageMv(const BatteryMetrics& sample)
{
    return max(sample.bqVoltageMv, sample.pmuBattVoltageMv);
}

const char* stateLabel(const UiState state)
{
    switch (state) {
        case UiState::Init:         return "INIT";
        case UiState::Charging:     return "CHARGING";
        case UiState::ChargeDone:   return "CHARGE DONE";
        case UiState::Discharging:  return "DISCHARGING";
        case UiState::Idle:         return "IDLE";
        case UiState::BqConfigWarn: return "BQ CFG WARN";
        case UiState::Error:        return "ERROR";
        default:                    return "?";
    }
}

uint16_t stateColor(const UiState state)
{
    switch (state) {
        case UiState::Init:         return TFT_CYAN;
        case UiState::Charging:     return TFT_GREEN;
        case UiState::ChargeDone:   return TFT_YELLOW;
        case UiState::Discharging:  return TFT_ORANGE;
        case UiState::Idle:         return TFT_LIGHTGREY;
        case UiState::BqConfigWarn: return TFT_RED;
        case UiState::Error:        return TFT_RED;
        default:                    return TFT_WHITE;
    }
}

const char* busStatusLabel(const uint8_t status)
{
    switch (static_cast<XPowersPPM::BusStatus>(status)) {
        case XPowersPPM::BUS_STATE_NOINPUT:             return "No input";
        case XPowersPPM::BUS_STATE_USB_SDP:             return "USB SDP";
        case XPowersPPM::BUS_STATE_USB_CDP:             return "USB CDP";
        case XPowersPPM::BUS_STATE_USB_DCP:             return "USB DCP";
        case XPowersPPM::BUS_STATE_HVDCP:               return "HVDCP";
        case XPowersPPM::BUS_STATE_ADAPTER:             return "Adapter";
        case XPowersPPM::BUS_STATE_NO_STANDARD_ADAPTER: return "Adapter";
        case XPowersPPM::BUS_STATE_OTG:                 return "OTG";
        default:                                        return "Unknown";
    }
}

const char* chargeStatusLabel(const uint8_t status)
{
    switch (static_cast<XPowersPPM::ChargeStatus>(status)) {
        case XPowersPPM::CHARGE_STATE_NO_CHARGE:   return "Not Charging";
        case XPowersPPM::CHARGE_STATE_PRE_CHARGE:  return "Pre-charge";
        case XPowersPPM::CHARGE_STATE_FAST_CHARGE: return "Fast Charging";
        case XPowersPPM::CHARGE_STATE_DONE:        return "Done";
        case XPowersPPM::CHARGE_STATE_UNKOWN:
        default:                                   return "Unknown";
    }
}

const char* menuChargeStatusLabel(const uint8_t status, const bool vbusPresent)
{
    switch (static_cast<XPowersPPM::ChargeStatus>(status)) {
        case XPowersPPM::CHARGE_STATE_PRE_CHARGE:  return "Pre-charge";
        case XPowersPPM::CHARGE_STATE_FAST_CHARGE: return "Charging";
        case XPowersPPM::CHARGE_STATE_DONE:        return "Charge Done";
        case XPowersPPM::CHARGE_STATE_NO_CHARGE:   return vbusPresent ? "Idle" : "Discharging";
        case XPowersPPM::CHARGE_STATE_UNKOWN:
        default:                                   return "Unknown";
    }
}

uint16_t menuChargeStatusColor(const uint8_t status, const bool vbusPresent)
{
    switch (static_cast<XPowersPPM::ChargeStatus>(status)) {
        case XPowersPPM::CHARGE_STATE_PRE_CHARGE:
            return TFT_ORANGE;
        case XPowersPPM::CHARGE_STATE_FAST_CHARGE:
            return TFT_GREEN;
        case XPowersPPM::CHARGE_STATE_DONE:
            return TFT_YELLOW;
        case XPowersPPM::CHARGE_STATE_NO_CHARGE:
            return vbusPresent ? TFT_CYAN : TFT_ORANGE;
        case XPowersPPM::CHARGE_STATE_UNKOWN:
        default:
            return TFT_LIGHTGREY;
    }
}

String currentDetail()
{
    if (detailExpiresAtMs && millis() < detailExpiresAtMs) {
        return transientDetail;
    }
    return bqConfigDetail;
}

void setTransientDetail(const String& detail, const uint32_t durationMs = kTransientDetailMs)
{
    transientDetail = detail;
    detailExpiresAtMs = millis() + durationMs;
    screenDirty = true;
}

void requestImmediatePoll()
{
    const unsigned long now = millis();
    lastPollAtMs = (now > kPollIntervalMs) ? (now - kPollIntervalMs) : 0;
}

String formatSignedMilliamp(const int16_t value)
{
    return String(value) + " mA";
}

String formatVoltage(const uint16_t value)
{
    if (!value) {
        return "-";
    }
    return String(value) + " mV";
}

String formatTimePair(const uint16_t tteMin, const uint16_t ttfMin)
{
    auto part = [](const uint16_t value) -> String {
        if (!value || value == 65535U) {
            return "-";
        }
        return String(value);
    };
    return part(tteMin) + " / " + part(ttfMin) + " min";
}

String formatTemp(const int16_t deciC)
{
    const bool negative = deciC < 0;
    const int16_t magnitude = negative ? -deciC : deciC;
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%s%d.%d C", negative ? "-" : "", magnitude / 10, magnitude % 10);
    return String(buffer);
}

String formatCapacityTriplet(const BatteryMetrics& sample)
{
    return String(sample.remainMah) + "/" + String(sample.fullMah) + "/" + String(sample.designMah) + " mAh";
}

void drawHeader()
{
    drawPageHeader("Battery Charge/Discharge Test", TFT_GREEN);
}

void drawFooter()
{
    const int16_t footerY = tft.height() - kFooterH;
    tft.fillRect(0, footerY, tft.width(), kFooterH, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    const char* text = focus == FocusItem::Back
        ? "ENC press=back  USER hold=shutdown"
        : "TURN=BACK  USER hold=shutdown";
    tft.drawString(text, 4, footerY + 3, 1);
    drawBackButton(focus == FocusItem::Back);
}

void drawStaticFrame()
{
    tft.fillScreen(TFT_BLACK);
    drawHeader();
    drawFooter();

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("SOC", kLeftX, kRow0, 1);
    tft.drawString("VBAT", kLeftX, kRow0 + kRowGap, 1);
    tft.drawString("IBAT", kLeftX, kRow0 + kRowGap * 2, 1);
    tft.drawString("R/F/D", kLeftX, kRow0 + kRowGap * 3, 1);
    tft.drawString("SOH/T", kLeftX, kRow0 + kRowGap * 4, 1);

    tft.drawString("VBUS/VSYS", kRightX, kRow0, 1);
    tft.drawString("PMU Bus", kRightX, kRow0 + kRowGap, 1);
    tft.drawString("Charge", kRightX, kRow0 + kRowGap * 2, 1);
    tft.drawString("Cfg V/P/F", kRightX, kRow0 + kRowGap * 3, 1);
    tft.drawString("TTE/TTF", kRightX, kRow0 + kRowGap * 4, 1);

    frameDrawn = true;
}

void drawStatusArea()
{
    tft.fillRect(kStateX, kStateY, 220, 26, TFT_BLACK);
    tft.setTextColor(stateColor(uiState), TFT_BLACK);
    tft.drawString(stateLabel(uiState), kStateX, kStateY, 4);
    lastDrawnUiState = uiState;

    const String detail = currentDetail();
    tft.fillRect(kDetailX, kDetailY, 304, 10, TFT_BLACK);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(detail, kDetailX, kDetailY, 1);
    lastDrawnDetail = detail;
}

void drawMetricValues()
{
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(metrics.soc) + "%   ", kLeftValueX, kRow0, 1);
    tft.drawString(formatVoltage(metrics.bqVoltageMv) + "   ", kLeftValueX, kRow0 + kRowGap, 1);
    tft.drawString(formatSignedMilliamp(metrics.ibatMa) + "   ", kLeftValueX, kRow0 + kRowGap * 2, 1);
    tft.drawString(formatCapacityTriplet(metrics) + "   ", kLeftValueX, kRow0 + kRowGap * 3, 1);
    tft.drawString(String(metrics.soh) + "% / " + formatTemp(metrics.tempDeciC) + "   ",
                   kLeftValueX, kRow0 + kRowGap * 4, 1);

    tft.drawString(formatVoltage(metrics.vbusMv) + " / " + formatVoltage(metrics.vsysMv) + "   ",
                   kRightValueX, kRow0, 1);
    tft.drawString(String(busStatusLabel(metrics.busStatus)) + "   ", kRightValueX, kRow0 + kRowGap, 1);
    tft.drawString(String(chargeStatusLabel(metrics.chargeStatus)) + " " + String(metrics.chargeCurrentMa) + "mA   ",
                   kRightValueX, kRow0 + kRowGap * 2, 1);
    tft.drawString(String(kChargeTargetVoltageMv) + "/" + String(kPrechargeCurrentMa) + "/" +
                   String(kFastChargeCurrentMa) + "   ",
                   kRightValueX, kRow0 + kRowGap * 3, 1);
    tft.drawString(formatTimePair(metrics.tteMin, metrics.ttfMin) + "   ",
                   kRightValueX, kRow0 + kRowGap * 4, 1);
}

void redrawScreen()
{
    if (!frameDrawn) {
        drawStaticFrame();
    } else {
        drawFooter();
    }
    drawStatusArea();
    if (hasMetrics) {
        drawMetricValues();
    }
    board_spi_deselect_all();
    lastDrawnMetrics = metrics;
}

void setErrorState(const String& detail)
{
    uiState = UiState::Error;
    pendingUiState = UiState::Error;
    pendingUiStateSinceMs = 0;
    bqConfigDetail = detail;
    transientDetail = "";
    detailExpiresAtMs = 0;
    screenDirty = true;
}

void printBqChargeConfigTargets()
{
    Serial.print(kFastChargeCurrentMa);
    Serial.print(F("/"));
    Serial.print(kChargeTargetVoltageMv);
    Serial.print(F("/"));
    Serial.print(kTerminationCurrentMa);
    Serial.print(F("/"));
    Serial.print(kBqChargeTerminationVoltageMv);
    Serial.print(F("/"));
    Serial.print(kBqChargeDetectThresholdMa);
    Serial.print(F("/"));
    Serial.println(kBqQuitCurrentMa);
}

bool verifyGaugeChargeParameters()
{
    BQ27220DMData chargeConfig[] = {
        makeDmU16Entry(BQ27220DMAddressChargingChargingCurrent, kFastChargeCurrentMa),
        makeDmU16Entry(BQ27220DMAddressChargingChargingVoltage, kChargeTargetVoltageMv),
        makeDmU16Entry(BQ27220DMAddressChargingTaperCurrent, kTerminationCurrentMa),
        makeDmU16Entry(BQ27220DMAddressGasGaugingCEDVProfile1ChargeTerminationVoltage, kBqChargeTerminationVoltageMv),
        makeDmU16Entry(BQ27220DMAddressConfigurationCurrentThresholdsChargeDetectThreshold, kBqChargeDetectThresholdMa),
        makeDmU16Entry(BQ27220DMAddressConfigurationCurrentThresholdsQuitCurrent, kBqQuitCurrentMa),
        makeDmEndEntry(),
    };

    BQ27220OperationStatus operationStatus = {};
    gauge.getOperationStatus(&operationStatus);
    const bool reseal = operationStatus.reg.SEC == Bq27220OperationStatusSecSealed;

    if (!gauge.unsealAccess()) {
        Serial.println(F("[BAT] Failed to unseal BQ27220 for verify."));
        return false;
    }
    if (!gauge.fullAccess()) {
        Serial.println(F("[BAT] Failed to enter BQ27220 full-access verify mode."));
        return false;
    }

    const bool ok = gauge.dateMemoryCheck(chargeConfig, false);
    if (reseal) {
        gauge.sealAccess();
    }
    return ok;
}

bool gaugeProvisioningLooksValidReadOnly()
{
    const uint16_t chipId = gauge.getDeviceNumber();
    if (chipId != BQ27220_DEVICE_ID) {
        return false;
    }

    return gauge.getDesignCapacity() == kBatteryCapacityMah &&
           gauge.getFullChargeCapacity() == kBatteryCapacityMah &&
           gauge.getChargeVoltageMax() == kChargeTargetVoltageMv &&
           gauge.getChargeCurrent() == kFastChargeCurrentMa;
}

bool initGauge()
{
    gauge.setDefaultCapacity(kBatteryCapacityMah);
    delay(kGaugeStartupSettleMs);

    bool initOk = false;
    for (uint8_t attempt = 1; attempt <= kGaugeInitAttempts; ++attempt) {
        if (gauge.init()) {
            initOk = true;
            break;
        }
        Serial.print(F("[BAT] BQ27220 init retry "));
        Serial.print(attempt);
        Serial.print(F("/"));
        Serial.println(kGaugeInitAttempts);
        delay(kGaugeInitRetryDelayMs);
    }

    if (!initOk) {
        if (!gaugeProvisioningLooksValidReadOnly()) {
            Serial.println(F("[BAT] BQ27220 init failed."));
            return false;
        }
        Serial.println(F("[BAT] BQ27220 unlock check failed after USB reset, using existing gauge config."));
        bqConfigWarning = false;
        bqConfigDetail = "BQ ready (skip unlock)";
    }

    const uint16_t chipId = gauge.getDeviceNumber();
    if (chipId != BQ27220_DEVICE_ID) {
        Serial.print(F("[BAT] Unexpected BQ27220 chip ID: 0x"));
        Serial.println(chipId, HEX);
        return false;
    }

    if (gauge.getDesignCapacity() != kBatteryCapacityMah ||
        gauge.getFullChargeCapacity() != kBatteryCapacityMah) {
        bqConfigWarning = true;
        bqConfigDetail = "BQ cap verify mismatch";
        Serial.println(F("[BAT] BQ27220 capacity verify mismatch."));
        return false;
    }

    if (initOk && !verifyGaugeChargeParameters()) {
        bqConfigWarning = true;
        bqConfigDetail = "BQ charge cfg mismatch";
        Serial.println(F("[BAT] BQ27220 charge parameter verify mismatch."));
        return false;
    }

    if (initOk) {
        bqConfigWarning = false;
        bqConfigDetail = "BQ CFG OK";
        Serial.print(F("[BAT] BQ charge cfg verified I/V/Taper/TermV/Detect/Quit: "));
        printBqChargeConfigTargets();
    } else {
        Serial.print(F("[BAT] BQ read-only cfg verified V/I: "));
        Serial.print(gauge.getChargeVoltageMax());
        Serial.print(F("/"));
        Serial.println(gauge.getChargeCurrent());
    }

    return true;
}

void armUsbSourceDetection()
{
    pmu.enableAutomaticInputDetection();
    pmu.enableInputDetection();
}

bool configurePmuChargePath(const bool resetRegisters)
{
    if (resetRegisters) {
        pmu.resetDefault();
        delay(20);
    }

    pmu.disableWatchdog();
    pmu.exitHizMode();
    pmu.disableOTG();
    pmu.enableBatterPowerPath();
    pmu.disableCurrentLimitPin();

    if (!pmu.setInputCurrentLimit(kInputCurrentSdpMa)) {
        Serial.println(F("[BAT] Failed to set initial input current limit."));
        return false;
    }
    appliedInputCurrentMa = kInputCurrentSdpMa;

    if (!pmu.setSysPowerDownVoltage(kSysPowerDownVoltageMv)) {
        Serial.println(F("[BAT] Failed to set SYS power-down voltage."));
        return false;
    }
    if (!pmu.setChargeTargetVoltage(kChargeTargetVoltageMv)) {
        Serial.println(F("[BAT] Failed to set charge target voltage."));
        return false;
    }
    if (!pmu.setPrechargeCurr(kPrechargeCurrentMa)) {
        Serial.println(F("[BAT] Failed to set precharge current."));
        return false;
    }
    if (!pmu.setTerminationCurr(kTerminationCurrentMa)) {
        Serial.println(F("[BAT] Failed to set termination current."));
        return false;
    }

    pmu.setBatteryRechargeThresholdOffset(
        kRechargeThresholdOffsetMv >= 200 ? XPowersPPM::RECHARGE_OFFSET_200MV
                                          : XPowersPPM::RECHARGE_OFFSET_100MV);

    if (!pmu.setChargerConstantCurr(kFastChargeCurrentMa)) {
        Serial.println(F("[BAT] Failed to set fast charge current."));
        return false;
    }
    if (!pmu.enableMeasure()) {
        Serial.println(F("[BAT] Failed to enable ADC measurement."));
        return false;
    }

    pmu.enableChargingTermination();
    pmu.setFastChargeTimer(XPowersPPM::FAST_CHARGE_TIMER_12H);
    pmu.enableChargingSafetyTimer();
    pmu.enableCharge();

    const bool staticConfigMismatch =
        pmu.getChargeTargetVoltage() != kChargeTargetVoltageMv ||
        pmu.getPrechargeCurr() != kPrechargeCurrentMa ||
        pmu.getChargerConstantCurr() != kFastChargeCurrentMa ||
        readTerminationCurrentMa() != kTerminationCurrentMa ||
        readRechargeThresholdOffsetMv() != kRechargeThresholdOffsetMv ||
        !pmu.isEnableChargingSafetyTimer() ||
        pmu.isHizMode() ||
        !pmu.isEnableCharge();

    if (staticConfigMismatch) {
        Serial.println(F("[BAT] BQ25896 parameter verify mismatch."));
        return false;
    }

    armUsbSourceDetection();
    return true;
}

bool configurePmu()
{
    if (!pmu.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BQ25896_SLAVE_ADDRESS)) {
        Serial.println(F("[BAT] BQ25896 init failed."));
        return false;
    }
    return configurePmuChargePath(true);
}

bool metricsChanged(const BatteryMetrics& a, const BatteryMetrics& b)
{
    return a.soc != b.soc ||
           a.bqVoltageMv != b.bqVoltageMv ||
           a.ibatMa != b.ibatMa ||
           a.remainMah != b.remainMah ||
           a.fullMah != b.fullMah ||
           a.designMah != b.designMah ||
           a.soh != b.soh ||
           a.tempDeciC != b.tempDeciC ||
           a.tteMin != b.tteMin ||
           a.ttfMin != b.ttfMin ||
           a.pmuBattVoltageMv != b.pmuBattVoltageMv ||
           a.vbusMv != b.vbusMv ||
           a.vsysMv != b.vsysMv ||
           a.chargeCurrentMa != b.chargeCurrentMa ||
           a.busStatus != b.busStatus ||
           a.chargeStatus != b.chargeStatus ||
           a.chargeEnabled != b.chargeEnabled ||
           a.powerGood != b.powerGood ||
           a.hizMode != b.hizMode ||
           a.bqFullChargeDetected != b.bqFullChargeDetected ||
           a.isDischarging != b.isDischarging ||
           a.vbusPresent != b.vbusPresent;
}

UiState evaluateObservedUiState(const BatteryMetrics& sample)
{
    if (bqConfigWarning) {
        return UiState::BqConfigWarn;
    }

    const auto chargeState = static_cast<XPowersPPM::ChargeStatus>(sample.chargeStatus);
    const bool externalPower = hasExternalPower(sample);
    const bool bqChargeDone = sample.bqFullChargeDetected || sample.soc >= kChargeDoneSocThreshold;
    const bool activelyCharging = hasChargingEvidence(sample);
    const bool keepCharging = externalPower &&
                              sample.chargeEnabled &&
                              !bqChargeDone &&
                              lastChargingEvidenceAtMs != 0 &&
                              (millis() - lastChargingEvidenceAtMs) < kChargingStateHoldMs;
    const bool chargeDoneLikely = sample.chargeEnabled && bqChargeDone && !sample.isDischarging;

    if (chargeState == XPowersPPM::CHARGE_STATE_DONE && bqChargeDone) {
        return UiState::ChargeDone;
    }
    if (externalPower) {
        if (activelyCharging || keepCharging) {
            return UiState::Charging;
        }
        if (chargeDoneLikely) {
            return UiState::ChargeDone;
        }
        return UiState::Idle;
    }
    if (sample.ibatMa <= kDischargeCurrentThresholdMa || sample.isDischarging) {
        return UiState::Discharging;
    }
    return UiState::Idle;
}

void forceUiState(const UiState state)
{
    pendingUiState = state;
    pendingUiStateSinceMs = 0;
    if (uiState != state) {
        uiState = state;
        screenDirty = true;
    }
}

void updateUiState(const UiState observedState)
{
    const unsigned long now = millis();

    if (uiState == UiState::Init || observedState == UiState::BqConfigWarn || observedState == UiState::Error) {
        pendingUiState = observedState;
        pendingUiStateSinceMs = 0;
        if (uiState != observedState) {
            uiState = observedState;
            screenDirty = true;
        }
        return;
    }

    if (observedState == uiState) {
        pendingUiState = observedState;
        pendingUiStateSinceMs = 0;
        return;
    }

    if (observedState != pendingUiState) {
        pendingUiState = observedState;
        pendingUiStateSinceMs = now;
        return;
    }

    if (pendingUiStateSinceMs && (now - pendingUiStateSinceMs) >= kUiStateDebounceMs) {
        uiState = observedState;
        pendingUiStateSinceMs = 0;
        screenDirty = true;
    }
}

bool maybeRestartChargeTopOff()
{
    if (!metrics.vbusPresent || !metrics.chargeEnabled) {
        return false;
    }

    const auto chargeState = static_cast<XPowersPPM::ChargeStatus>(metrics.chargeStatus);
    if (chargeState != XPowersPPM::CHARGE_STATE_DONE &&
        chargeState != XPowersPPM::CHARGE_STATE_NO_CHARGE) {
        return false;
    }
    if (metrics.bqFullChargeDetected || metrics.soc >= kChargeDoneSocThreshold) {
        return false;
    }
    if (metrics.chargeCurrentMa > kChargeCurrentIntoBatteryThresholdMa) {
        return false;
    }
    if (lastChargeTopOffAttemptMs != 0 &&
        (millis() - lastChargeTopOffAttemptMs) < kChargeTopOffRetryMs) {
        return false;
    }
    if (effectiveBatteryVoltageMv(metrics) + kChargeTopOffRestartMarginMv >= kChargeTargetVoltageMv) {
        return false;
    }

    lastChargeTopOffAttemptMs = millis();
    Serial.print(F("[BAT] Restarting charge top-off at "));
    Serial.print(effectiveBatteryVoltageMv(metrics));
    Serial.println(F(" mV because BQ is not full yet."));
    pmu.disableCharge();
    delay(20);
    pmu.enableCharge();
    setTransientDetail("Charge top-off restart");
    return true;
}

bool applyDynamicInputCurrentLimit()
{
    const auto bus = static_cast<XPowersPPM::BusStatus>(metrics.busStatus);
    int16_t desired = -1;

    switch (bus) {
        case XPowersPPM::BUS_STATE_USB_SDP:
            desired = kInputCurrentSdpMa;
            break;
        case XPowersPPM::BUS_STATE_USB_CDP:
        case XPowersPPM::BUS_STATE_USB_DCP:
        case XPowersPPM::BUS_STATE_HVDCP:
        case XPowersPPM::BUS_STATE_ADAPTER:
        case XPowersPPM::BUS_STATE_NO_STANDARD_ADAPTER:
            desired = kInputCurrentAdapterMa;
            break;
        case XPowersPPM::BUS_STATE_NOINPUT:
        case XPowersPPM::BUS_STATE_OTG:
        default:
            desired = -1;
            break;
    }

    if (desired < 0) {
        if (appliedInputCurrentMa != -1) {
            appliedInputCurrentMa = -1;
            Serial.println(F("[BAT] No VBUS input current limit applied."));
        }
        return true;
    }
    if (desired == appliedInputCurrentMa) {
        return true;
    }
    if (!pmu.setInputCurrentLimit(desired)) {
        Serial.print(F("[BAT] Failed to set input current limit to "));
        Serial.println(desired);
        return false;
    }

    appliedInputCurrentMa = desired;
    Serial.print(F("[BAT] Input current limit set to "));
    Serial.print(desired);
    Serial.print(F(" mA for "));
    Serial.println(busStatusLabel(metrics.busStatus));
    return true;
}

void handleVbusTransition(const BatteryMetrics& previous, BatteryMetrics& current)
{
    if (current.vbusPresent == previous.vbusPresent) {
        return;
    }

    appliedInputCurrentMa = -1;
    lastExternalPowerSeenAtMs = 0;
    lastChargeTopOffAttemptMs = 0;

    if (current.vbusPresent) {
        pmu.exitHizMode();
        current.hizMode = pmu.isHizMode();
        armUsbSourceDetection();
        pmu.enableCharge();
        current.chargeEnabled = pmu.isEnableCharge();
        setTransientDetail("USB connected");
    } else {
        latchedInputBusStatus = static_cast<uint8_t>(XPowersPPM::BUS_STATE_NOINPUT);
        lastChargingEvidenceAtMs = 0;
        setTransientDetail("USB disconnected");
    }

    requestImmediatePoll();
}

bool refreshMetrics()
{
    if (!gaugeReady || !pmuReady) {
        return false;
    }

    const bool hadMetrics = hasMetrics;
    const BatteryMetrics previous = metrics;
    BatteryMetrics next = {};
    BQ27220BatteryStatus batteryStatus = {};
    const unsigned long now = millis();

    gauge.getBatteryStatus(&batteryStatus);

    next.soc = gauge.getStateOfCharge();
    next.bqVoltageMv = gauge.getVoltage();
    next.ibatMa = gauge.getCurrent();
    next.remainMah = gauge.getRemainingCapacity();
    next.fullMah = gauge.getFullChargeCapacity();
    next.designMah = gauge.getDesignCapacity();
    next.soh = gauge.getStateOfHealth();
    next.tempDeciC = gaugeTemperatureDeciC();
    next.tteMin = gauge.getTimeToEmpty();
    next.ttfMin = gauge.getTimeToFull();
    next.pmuBattVoltageMv = pmu.getBattVoltage();
    next.vbusMv = pmu.getVbusVoltage();
    next.vsysMv = pmu.getSystemVoltage();
    next.busStatus = static_cast<uint8_t>(pmu.getBusStatus());
    next.chargeStatus = static_cast<uint8_t>(pmu.chargeStatus());
    next.chargeCurrentMa = readPmuChargeCurrentMa(next.chargeStatus);
    next.chargeEnabled = pmu.isEnableCharge();
    next.powerGood = pmu.isPowerGood() && (next.vbusMv >= kVbusPresentThresholdMv);
    next.hizMode = pmu.isHizMode();
    next.bqFullChargeDetected = batteryStatus.reg.FC;
    next.isDischarging = batteryStatus.reg.DSG;

    const bool rawBusHasPower = isUsableInputBusStatus(next.busStatus);
    const bool vbusMeasuredPresent = next.vbusMv >= kVbusPresentThresholdMv;
    const bool rawExternalPower = rawBusHasPower || vbusMeasuredPresent;
    const bool pmuChargingEvidence = hasChargingEvidence(next);

    if (rawExternalPower) {
        lastExternalPowerSeenAtMs = now;
    }

    const bool recentExternalPower = lastExternalPowerSeenAtMs != 0 &&
                                     (now - lastExternalPowerSeenAtMs) < kExternalPowerHoldMs;
    next.vbusPresent = rawExternalPower || recentExternalPower || pmuChargingEvidence;

    if (rawBusHasPower) {
        latchedInputBusStatus = next.busStatus;
    } else if (!next.vbusPresent) {
        latchedInputBusStatus = static_cast<uint8_t>(XPowersPPM::BUS_STATE_NOINPUT);
        lastExternalPowerSeenAtMs = 0;
    }

    if (!rawBusHasPower && next.vbusPresent && isUsableInputBusStatus(latchedInputBusStatus)) {
        next.busStatus = latchedInputBusStatus;
    }

    if (next.vbusPresent && next.hizMode) {
        pmu.exitHizMode();
        next.hizMode = pmu.isHizMode();
        setTransientDetail("BQ25896 HIZ cleared");
        requestImmediatePoll();
    }

    metrics = next;
    hasMetrics = true;

    if (!hasExternalPower(metrics)) {
        lastChargingEvidenceAtMs = 0;
    } else if (hasChargingEvidence(metrics)) {
        lastChargingEvidenceAtMs = millis();
    }

    const bool vbusTransition = hadMetrics && metrics.vbusPresent != previous.vbusPresent;
    if (vbusTransition) {
        handleVbusTransition(previous, metrics);
    }

    if (!applyDynamicInputCurrentLimit()) {
        setTransientDetail("BQ25896 input-limit update failed");
    }

    (void)maybeRestartChargeTopOff();
    const UiState observedState = evaluateObservedUiState(metrics);
    if (vbusTransition) {
        forceUiState(observedState);
    } else {
        updateUiState(observedState);
    }

    if (!hadMetrics || metricsChanged(metrics, lastDrawnMetrics)) {
        screenDirty = true;
    }
    return true;
}

bool pmuUsbPresent()
{
    return pmu.isVbusIn() || (pmu.getVbusVoltage() >= kVbusPresentThresholdMv);
}

bool pmuNeedsChargeRecovery()
{
    if (!pmuUsbPresent()) {
        return false;
    }

    const uint16_t batteryMv = pmu.getBattVoltage();
    const uint16_t targetMv = pmu.getChargeTargetVoltage();
    const bool batteryNeedsCharge = (batteryMv == 0) ||
                                    (targetMv == 0) ||
                                    (batteryMv + 96 < targetMv);
    const bool chargePathInvalid = pmu.isHizMode() ||
                                   pmu.isEnableOTG() ||
                                   !pmu.isEnableCharge();
    const bool chargeStateMissing = pmu.chargeStatus() == XPowersPPM::CHARGE_STATE_NO_CHARGE;
    const bool powerNotGood = !pmu.isPowerGood();
    const bool batteryStillDischarging = gauge.getAverageCurrent() <= kRecoveryDischargeCurrentMa;

    return chargePathInvalid ||
           (batteryNeedsCharge && (chargeStateMissing || powerNotGood || batteryStillDischarging));
}

void pmuServiceChargeRecovery()
{
    const bool usbPresent = pmuUsbPresent();
    if (usbPresent && !chargeRecoveryUsbWasPresent) {
        Serial.println(F("[BAT] VBUS inserted, restoring BQ25896 charge path."));
        if (configurePmuChargePath(true)) {
            lastChargeRecoveryMs = millis();
            requestImmediatePoll();
        }
    } else if (usbPresent && pmuNeedsChargeRecovery()) {
        const unsigned long now = millis();
        if ((lastChargeRecoveryMs == 0) ||
            (now - lastChargeRecoveryMs >= kChargeRecoveryRetryIntervalMs)) {
            Serial.printf("[BAT] BQ25896 recovery: bus=%d chg=%d vbus=%umV vbat=%umV avg=%dmA hiz=%d otg=%d enchg=%d pg=%d\n",
                          static_cast<int>(pmu.getBusStatus()),
                          static_cast<int>(pmu.chargeStatus()),
                          pmu.getVbusVoltage(),
                          pmu.getBattVoltage(),
                          gauge.getAverageCurrent(),
                          static_cast<int>(pmu.isHizMode()),
                          static_cast<int>(pmu.isEnableOTG()),
                          static_cast<int>(pmu.isEnableCharge()),
                          static_cast<int>(pmu.isPowerGood()));
            if (configurePmuChargePath(true)) {
                lastChargeRecoveryMs = now;
                requestImmediatePoll();
                setTransientDetail("Charge path restored");
            }
        }
    }

    if (!usbPresent) {
        lastChargeRecoveryMs = 0;
    }
    chargeRecoveryUsbWasPresent = usbPresent;
}

void printStatus()
{
    if (!hasMetrics) {
        Serial.println(F("[BAT] Metrics not ready yet."));
        return;
    }

    Serial.println();
    Serial.print(F("[BAT] UI State:        "));
    Serial.println(stateLabel(uiState));
    Serial.print(F("[BAT] Detail:          "));
    Serial.println(currentDetail());
    Serial.print(F("[BAT] BQ Voltage:      "));
    Serial.print(metrics.bqVoltageMv);
    Serial.println(F(" mV"));
    Serial.print(F("[BAT] BQ Current:      "));
    Serial.print(metrics.ibatMa);
    Serial.println(F(" mA"));
    Serial.print(F("[BAT] SOC / SOH:       "));
    Serial.print(metrics.soc);
    Serial.print(F("% / "));
    Serial.print(metrics.soh);
    Serial.println(F("%"));
    Serial.print(F("[BAT] VBUS / VSYS:     "));
    Serial.print(metrics.vbusMv);
    Serial.print(F(" / "));
    Serial.print(metrics.vsysMv);
    Serial.println(F(" mV"));
    Serial.print(F("[BAT] PMU Bus:         "));
    Serial.println(busStatusLabel(metrics.busStatus));
    Serial.print(F("[BAT] Charge State:    "));
    Serial.println(chargeStatusLabel(metrics.chargeStatus));
}

void refreshMenuPreviewCache()
{
    const uint32_t now = millis();
    if (lastPreviewRefreshMs != 0 && (now - lastPreviewRefreshMs) < 1500U) {
        return;
    }
    lastPreviewRefreshMs = now;

    previewSocValid = false;
    previewChargeValid = false;
    previewVbusPresent = false;
    previewChargeStatus = static_cast<uint8_t>(XPowersPPM::CHARGE_STATE_UNKOWN);

    if (!i2cDevicePresent(BOARD_I2C_ADDR_2) || !i2cDevicePresent(BOARD_I2C_ADDR_3)) {
        return;
    }

    gauge.setDefaultCapacity(kBatteryCapacityMah);
    if (gauge.init()) {
        previewSoc = gauge.getStateOfCharge();
        previewSocValid = true;
    }

    if (pmu.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BQ25896_SLAVE_ADDRESS)) {
        previewChargeStatus = static_cast<uint8_t>(pmu.chargeStatus());
        previewVbusPresent = pmu.isVbusIn() || (pmu.getVbusVoltage() >= kVbusPresentThresholdMv);
        previewChargeValid = true;
    }
}

void updateUserButton()
{
    const bool rawPressed = (digitalRead(BOARD_USER_KEY) == LOW);
    const uint32_t now = millis();

    if (rawPressed != userButton.pressed && (now - userButton.lastChangeMs) >= kDebounceMs) {
        userButton.lastChangeMs = now;
        userButton.pressed = rawPressed;

        if (rawPressed) {
            userButton.pressedAtMs = now;
            userButton.longPressHandled = false;
        } else {
            userButton.longPressHandled = false;
        }
    }
}

void attemptShutdown()
{
    Serial.println(F("[BAT] Shutdown requested by USER KEY"));

    if (!pmuReady) {
        Serial.println(F("[BAT] BQ25896 unavailable, cannot shutdown."));
        setTransientDetail("BQ25896 unavailable");
        return;
    }
    if (hasExternalPower()) {
        Serial.println(F("[BAT] VBUS present, refusing shutdown. Unplug USB first."));
        setTransientDetail("VBUS present; unplug USB first");
        return;
    }

    setTransientDetail("Shutting down...");
    redrawScreen();
    delay(100);
    Serial.println(F("[BAT] BQ25896 shutdown now."));
    pmu.shutdown();
    while (true) {
        delay(1000);
    }
}

void handleLongPressShutdown()
{
    if (!userButton.pressed || userButton.longPressHandled) {
        return;
    }

    const uint32_t now = millis();
    if (now - userButton.pressedAtMs >= kShutdownHoldMs) {
        userButton.longPressHandled = true;
        attemptShutdown();
    }
}

void handleEncoderFocus()
{
    const int32_t delta = takeEncoderDelta(encSnapshot);
    if (delta == 0) {
        return;
    }

    int32_t next = static_cast<int32_t>(focus) + delta;
    next %= static_cast<int32_t>(FocusItem::kCount);
    if (next < 0) {
        next += static_cast<int32_t>(FocusItem::kCount);
    }
    focus = static_cast<FocusItem>(next);
    screenDirty = true;
}

void handleButtons()
{
    if (g.usrBtn.event) {
        g.usrBtn.event = false;
    }

    if (!takeEncoderButton()) {
        return;
    }

    if (focus == FocusItem::Back) {
        requestExitSubPage();
        return;
    }

    printStatus();
    requestImmediatePoll();
    (void)refreshMetrics();
    screenDirty = true;
}
}  // namespace

void init()
{
    focus = FocusItem::Metrics;
    encSnapshot = g.encRaw;
    screenDirty = true;
    frameDrawn = false;
    lastPollAtMs = 0;
    lastDrawMs = 0;
    detailExpiresAtMs = 0;
    pendingUiStateSinceMs = 0;
    lastChargeTopOffAttemptMs = 0;
    lastChargingEvidenceAtMs = 0;
    lastExternalPowerSeenAtMs = 0;
    lastChargeRecoveryMs = 0;
    appliedInputCurrentMa = -1;
    latchedInputBusStatus = static_cast<uint8_t>(XPowersPPM::BUS_STATE_NOINPUT);
    chargeRecoveryUsbWasPresent = false;
    hasMetrics = false;
    bqConfigWarning = false;
    transientDetail = "";
    bqConfigDetail = "Initializing BQ27220 / BQ25896";
    uiState = UiState::Init;
    pendingUiState = UiState::Init;

    userButton = {};
    userButton.pressed = (digitalRead(BOARD_USER_KEY) == LOW);
    userButton.lastChangeMs = millis();

    gaugeReady = i2cDevicePresent(BOARD_I2C_ADDR_2) && initGauge();
    if (!gaugeReady) {
        setErrorState("BQ27220 init/config failed");
    }

    pmuReady = i2cDevicePresent(BOARD_I2C_ADDR_3) && configurePmu();
    if (!pmuReady) {
        setErrorState("BQ25896 config failed");
    }

    if (gaugeReady && pmuReady) {
        if (!refreshMetrics()) {
            setErrorState("Initial battery refresh failed");
        } else {
            printStatus();
        }
    }
}

void update()
{
    handleEncoderFocus();
    handleButtons();
    updateUserButton();
    handleLongPressShutdown();

    if (gaugeReady && pmuReady) {
        pmuServiceChargeRecovery();
    }

    if (detailExpiresAtMs && millis() >= detailExpiresAtMs) {
        detailExpiresAtMs = 0;
        transientDetail = "";
        screenDirty = true;
    }

    const uint32_t now = millis();
    if (gaugeReady && pmuReady && (now - lastPollAtMs >= kPollIntervalMs)) {
        lastPollAtMs = now;
        (void)refreshMetrics();
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && now - lastDrawMs < kUiFrameIntervalMs)) {
        return;
    }

    board_prepare_display();
    gSubPageGfx.beginFrame();
    redrawScreen();
    gSubPageGfx.endFrame();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit() {}

String menuPreviewLine1()
{
    if (hasMetrics) {
        return String("SOC: ") + metrics.soc + "%";
    }

    refreshMenuPreviewCache();
    return previewSocValid ? String("SOC: ") + previewSoc + "%" : String("SOC: --");
}

String menuPreviewLine2()
{
    if (hasMetrics) {
        return String("CHG: ") + menuChargeStatusLabel(metrics.chargeStatus, metrics.vbusPresent);
    }

    refreshMenuPreviewCache();
    return previewChargeValid
        ? String("CHG: ") + menuChargeStatusLabel(previewChargeStatus, previewVbusPresent)
        : String("CHG: --");
}

uint16_t menuPreviewLine1Color()
{
    if (hasMetrics || previewSocValid) {
        return TFT_GREEN;
    }
    return TFT_LIGHTGREY;
}

uint16_t menuPreviewLine2Color()
{
    if (hasMetrics) {
        return menuChargeStatusColor(metrics.chargeStatus, metrics.vbusPresent);
    }

    refreshMenuPreviewCache();
    if (previewChargeValid) {
        return menuChargeStatusColor(previewChargeStatus, previewVbusPresent);
    }
    return TFT_LIGHTGREY;
}

}  // namespace page_battery
