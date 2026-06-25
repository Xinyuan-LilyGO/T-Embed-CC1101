#pragma once

constexpr uint32_t kMenuFrameIntervalMs = 20;
constexpr uint8_t kMenuPageSize = 10;

constexpr uint16_t kMenuBg        = 0x0841;
constexpr uint16_t kMenuPanel     = 0x10A2;
constexpr uint16_t kMenuPanelSoft = 0x18C3;
constexpr uint16_t kMenuPanelEdge = 0x2945;
constexpr uint16_t kMenuAccent    = 0x55FF;
constexpr uint16_t kMenuText      = TFT_WHITE;
constexpr uint16_t kMenuSubText   = 0x9CD3;
constexpr uint16_t kMenuFooter    = 0x1062;

constexpr int16_t kMenuHeaderH   = 24;
constexpr int16_t kMenuFooterY   = 152;
constexpr int16_t kMenuFooterH   = 18;
constexpr int16_t kMenuListX     = 8;
constexpr int16_t kMenuListY     = 32;
constexpr int16_t kMenuListW     = 176;
constexpr int16_t kMenuListH     = 118;
constexpr int16_t kMenuRowX      = 14;
constexpr int16_t kMenuRowStartY = 40;
constexpr int16_t kMenuRowW      = 164;
constexpr int16_t kMenuRowH      = 10;
constexpr int16_t kMenuRowPitch  = 11;
constexpr int16_t kMenuDetailX   = 192;
constexpr int16_t kMenuDetailY   = 32;
constexpr int16_t kMenuDetailW   = 120;
constexpr int16_t kMenuDetailH   = 118;

struct MainMenuInsight {
    const char* line1;
    const char* line2;
};

static const MainMenuInsight kMainMenuInsights[kPageCount] = {
    {"Gauge and PMU",   "Charge and USB"},
    {"SPI and RF path", "Packet and RSSI"},
    {"IR TX and RX",    "Preset carrier"},
    {"Audio self-test", "MIC & speaker"},
    {"PN532 detect",    "UID and state"},
    {"2.4 GHz radio",   "Burst and RX"},
    {"Mount and files", "Read/write test"},
    {"Scan AP list",    "Signal detail"},
    {"Pixels and demo", "Colors and motion"},
    {"Color preview",   "Brightness step"},
    {"Rotation sleep",  "Info and timer"},
};

uint8_t selectedMenuIndex()
{
    int32_t idx = static_cast<int32_t>(g.menuCursor);
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= static_cast<int32_t>(kPageCount)) {
        idx = static_cast<int32_t>(kPageCount) - 1;
    }
    return static_cast<uint8_t>(idx);
}

String clipMenuText(const String& text, const uint8_t maxChars)
{
    if (text.length() <= maxChars || maxChars < 4) {
        return text;
    }
    return text.substring(0, maxChars - 3) + "...";
}

uint8_t menuPageStart()
{
    if (kPageCount <= kMenuPageSize) {
        return 0;
    }
    return static_cast<uint8_t>((selectedMenuIndex() / kMenuPageSize) * kMenuPageSize);
}

uint8_t menuVisibleCount()
{
    const uint8_t start = menuPageStart();
    return min<uint8_t>(kMenuPageSize, kPageCount - start);
}

uint8_t menuPageNumber()
{
    return static_cast<uint8_t>(menuPageStart() / kMenuPageSize + 1);
}

uint8_t menuPageCount()
{
    return static_cast<uint8_t>((kPageCount + kMenuPageSize - 1) / kMenuPageSize);
}

template <typename Canvas>
void drawMenuBackground(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kMenuBg);
}

template <typename Canvas>
void drawMenuHeader(Canvas& gfx)
{
    const int16_t W = gfx.width();
    const uint8_t idx = selectedMenuIndex();

    gfx.fillRect(0, 0, W, kMenuHeaderH, kMenuPanel);
    gfx.drawFastHLine(0, kMenuHeaderH - 1, W, kMenuPanelEdge);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(kMenuText, kMenuPanel);
    gfx.drawString("Factory Test", 8, 4, 2);
    gfx.setTextColor(kMenuSubText, kMenuPanel);
    if (menuPageCount() > 1) {
        gfx.drawString(String("Page ") + menuPageNumber() + "/" + menuPageCount(), 108, 6, 1);
    } else {
        gfx.drawString("Select a module", 108, 6, 1);
    }

    char pageBuf[12];
    snprintf(pageBuf, sizeof(pageBuf), "%02u/%02u",
             static_cast<unsigned>(idx + 1),
             static_cast<unsigned>(kPageCount));
    gfx.setTextColor(kMenuAccent, kMenuPanel);
    gfx.drawRightString(pageBuf, W - 8, 6, 1);
}

template <typename Canvas>
void drawMenuList(Canvas& gfx)
{
    const uint8_t selected = selectedMenuIndex();
    const uint8_t start = menuPageStart();
    const uint8_t count = menuVisibleCount();

    gfx.fillRoundRect(kMenuListX, kMenuListY, kMenuListW, kMenuListH, 6, kMenuPanel);
    gfx.drawRoundRect(kMenuListX, kMenuListY, kMenuListW, kMenuListH, 6, kMenuPanelEdge);

    for (uint8_t row = 0; row < count; ++row) {
        const uint8_t i = start + row;
        const int16_t y = kMenuRowStartY + row * kMenuRowPitch;
        const bool isSelected = (selected == i);
        const uint16_t rowBg = isSelected ? kMenuPanelSoft : kMenuPanel;

        gfx.fillRoundRect(kMenuRowX, y, kMenuRowW, kMenuRowH, 4, rowBg);
        if (isSelected) {
            gfx.fillRect(kMenuRowX + 1, y + 1, 3, kMenuRowH - 2, kMenuAccent);
            gfx.drawRoundRect(kMenuRowX, y, kMenuRowW, kMenuRowH, 4, kMenuAccent);
        }

        char num[4];
        snprintf(num, sizeof(num), "%02u", static_cast<unsigned>(i + 1));
        gfx.setTextColor(isSelected ? kMenuAccent : kMenuSubText, rowBg);
        gfx.drawString(num, kMenuRowX + 8, y + 1, 1);

        gfx.setTextColor(kMenuText, rowBg);
        gfx.drawString(kPages[i].label, kMenuRowX + 28, y + 1, 1);
    }
}

template <typename Canvas>
void drawMenuDetail(Canvas& gfx)
{
    const uint8_t idx = selectedMenuIndex();
    const MainMenuInsight& info = kMainMenuInsights[idx];
    String line1 = info.line1;
    String line2 = info.line2;
    uint16_t line1Color = kMenuSubText;
    uint16_t line2Color = kMenuSubText;
    if (idx == 0) {
        line1 = page_battery::menuPreviewLine1();
        line2 = page_battery::menuPreviewLine2();
        line1Color = page_battery::menuPreviewLine1Color();
        line2Color = page_battery::menuPreviewLine2Color();
    }
    const int16_t x = kMenuDetailX;
    const int16_t y = kMenuDetailY;
    const int16_t w = kMenuDetailW;

    gfx.fillRoundRect(x, y, kMenuDetailW, kMenuDetailH, 6, kMenuPanel);
    gfx.drawRoundRect(x, y, kMenuDetailW, kMenuDetailH, 6, kMenuPanelEdge);

    gfx.setTextColor(kMenuSubText, kMenuPanel);
    gfx.drawString("Current", x + 8, y + 6, 1);

    gfx.setTextColor(kMenuText, kMenuPanel);
    gfx.drawString(clipMenuText(String(kPages[idx].label), 16).c_str(), x + 8, y + 20, 1);

    gfx.setTextColor(line1Color, kMenuPanel);
    gfx.drawString(clipMenuText(line1, 16).c_str(), x + 8, y + 34, 1);
    gfx.setTextColor(line2Color, kMenuPanel);
    gfx.drawString(clipMenuText(line2, 16).c_str(), x + 8, y + 44, 1);

    gfx.drawFastHLine(x + 8, y + 58, w - 16, kMenuPanelEdge);

    gfx.setTextColor(kMenuAccent, kMenuPanel);
    gfx.drawString("Rotation", x + 8, y + 66, 1);
    gfx.drawString("Sleep", x + 8, y + 80, 1);
    gfx.drawString("Dim", x + 8, y + 94, 1);

    gfx.setTextColor(kMenuText, kMenuPanel);
    gfx.drawRightString(clipMenuText(currentRotationLabel(), 10).c_str(), x + w - 8, y + 66, 1);
    gfx.drawRightString(clipMenuText(autoSleepPresetLabel(), 10).c_str(), x + w - 8, y + 80, 1);
    gfx.drawRightString(clipMenuText(autoDimTimeoutLabel(), 10).c_str(), x + w - 8, y + 94, 1);
}

template <typename Canvas>
void drawMenuFooter(Canvas& gfx)
{
    const int16_t W = gfx.width();

    gfx.fillRect(0, kMenuFooterY, W, kMenuFooterH, kMenuFooter);
    gfx.drawFastHLine(0, kMenuFooterY, W, kMenuPanelEdge);

    gfx.setTextColor(kMenuSubText, kMenuFooter);
    gfx.drawString("ENC scroll", 8, kMenuFooterY + 5, 1);
    gfx.drawCentreString("BOOT open", W / 2, kMenuFooterY + 5, 1);
    gfx.drawRightString("USER next", W - 8, kMenuFooterY + 5, 1);
}

template <typename Canvas>
void drawMenuUi(Canvas& gfx)
{
    drawMenuBackground(gfx);
    drawMenuHeader(gfx);
    drawMenuList(gfx);
    drawMenuDetail(gfx);
    drawMenuFooter(gfx);
}
