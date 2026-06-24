#pragma once

namespace page_setting {

namespace {
enum class Focus : uint8_t { Info, Rotation, SleepNow, AutoOff, Back, kCount };
Focus focus = Focus::Info;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
uint32_t lastInfoMs = 0;
int32_t encSnapshot = 0;
uint32_t freeHeap = 0;
uint32_t minHeap = 0;

void refreshInfo()
{
    freeHeap = ESP.getFreeHeap();
    minHeap = ESP.getMinFreeHeap();
    lastInfoMs = millis();
    screenDirty = true;
}

const char* focusLabel(const Focus f)
{
    switch (f) {
        case Focus::Info:      return "Device Info";
        case Focus::Rotation:  return "Rotation";
        case Focus::SleepNow:  return "Sleep";
        case Focus::AutoOff:   return "Auto Power";
        case Focus::Back:      return "Back";
        default:               return "";
    }
}

uint16_t focusColor(const Focus f)
{
    switch (f) {
        case Focus::Info:      return TFT_CYAN;
        case Focus::Rotation:  return TFT_YELLOW;
        case Focus::SleepNow:  return TFT_ORANGE;
        case Focus::AutoOff:   return TFT_GREEN;
        case Focus::Back:      return TFT_WHITE;
        default:               return TFT_LIGHTGREY;
    }
}

void drawMenuItem(const Focus item, const int16_t y)
{
    const bool selected = focus == item;
    const uint16_t bg = selected ? focusColor(item) : kUiCard;
    const uint16_t fg = selected ? TFT_BLACK : TFT_WHITE;
    tft.fillRoundRect(8, y, 104, 20, 6, bg);
    tft.drawRoundRect(8, y, 104, 20, 6, selected ? focusColor(item) : kUiEdge);
    tft.setTextColor(fg, bg);
    tft.drawString(focusLabel(item), 16, y + 6, 1);
}

void drawDetail()
{
    tft.fillRoundRect(120, 34, 192, 114, 8, kUiPanel);
    tft.drawRoundRect(120, 34, 192, 114, 8, focusColor(focus));
    tft.setTextColor(focusColor(focus), kUiPanel);
    tft.drawString(focusLabel(focus), 130, 44, 2);
    tft.setTextColor(TFT_WHITE, kUiPanel);

    switch (focus) {
        case Focus::Info:
            tft.drawString(String("Chip: ") + ESP.getChipModel(), 130, 66, 1);
            tft.drawString(String("CPU: ") + ESP.getCpuFreqMHz() + " MHz", 130, 80, 1);
            tft.drawString(String("Flash: ") + (ESP.getFlashChipSize() / (1024UL * 1024UL)) + " MB", 130, 94, 1);
            tft.drawString(String("Heap: ") + (freeHeap / 1024UL) + " KB", 130, 108, 1);
            tft.drawString(String("Min: ") + (minHeap / 1024UL) + " KB", 130, 122, 1);
            break;
        case Focus::Rotation:
            tft.drawString(currentRotationLabel(), 130, 74, 4);
            tft.setTextColor(kUiMuted, kUiPanel);
            tft.drawString("BOOT toggles 1 / 3", 130, 116, 1);
            break;
        case Focus::SleepNow:
            tft.drawString("SLEEP", 130, 74, 4);
            tft.setTextColor(kUiMuted, kUiPanel);
            tft.drawString("Wake with USER key", 130, 116, 1);
            break;
        case Focus::AutoOff:
            tft.drawString(autoSleepPresetLabel(), 130, 70, 4);
            tft.setTextColor(TFT_YELLOW, kUiPanel);
            tft.drawString(String("Dim: ") + autoDimTimeoutLabel(), 130, 112, 1);
            break;
        case Focus::Back:
            tft.drawString("BACK", 130, 74, 4);
            tft.setTextColor(kUiMuted, kUiPanel);
            tft.drawString("BOOT returns to menu", 130, 116, 1);
            break;
        case Focus::kCount:
            break;
    }
}
}  // namespace

void init()
{
    focus = Focus::Info;
    encSnapshot = g.encRaw;
    screenDirty = true;
    lastDrawMs = 0;
    refreshInfo();
}

void update()
{
    (void)takeUserButton();
    const int32_t delta = takeEncoderDelta(encSnapshot);
    if (delta != 0) {
        int32_t idx = static_cast<int32_t>(focus) + delta;
        idx %= static_cast<int32_t>(Focus::kCount);
        if (idx < 0) {
            idx += static_cast<int32_t>(Focus::kCount);
        }
        focus = static_cast<Focus>(idx);
        screenDirty = true;
    }
    if (takeEncoderButton()) {
        switch (focus) {
            case Focus::Info:
                refreshInfo();
                break;
            case Focus::Rotation:
                cycleDisplayRotation();
                screenDirty = true;
                break;
            case Focus::SleepNow:
                requestSystemSleep();
                break;
            case Focus::AutoOff:
                cycleAutoSleepPreset();
                screenDirty = true;
                break;
            case Focus::Back:
                requestExitSubPage();
                break;
            case Focus::kCount:
                break;
        }
    }
    if (millis() - lastInfoMs > 1500) {
        refreshInfo();
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && now - lastDrawMs < 80)) {
        return;
    }
    board_prepare_display();
    tft.fillScreen(kUiBg);
    drawPageHeader("Settings", TFT_CYAN);
    drawPageFooter("ENC=select  BOOT=action", focus == Focus::Back);
    drawMenuItem(Focus::Info, 34);
    drawMenuItem(Focus::Rotation, 58);
    drawMenuItem(Focus::SleepNow, 82);
    drawMenuItem(Focus::AutoOff, 106);
    drawMenuItem(Focus::Back, 130);
    drawDetail();
    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit() {}

}  // namespace page_setting
