#pragma once

namespace page_tft {

namespace {
enum class Demo : uint8_t {
    Summary = 0,
    ColorBars,
    Geometry,
    Text,
    kCount,
};

constexpr uint32_t kFrameMs = 80;

Demo demo = Demo::Summary;
bool backFocused = false;
bool screenDirty = true;
uint32_t lastDrawMs = 0;
uint32_t lastAdvanceMs = 0;
int32_t encSnapshot = 0;

void nextDemo()
{
    int32_t idx = static_cast<int32_t>(demo) + 1;
    idx %= static_cast<int32_t>(Demo::kCount);
    demo = static_cast<Demo>(idx);
    screenDirty = true;
    lastAdvanceMs = millis();
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
void drawHeader(Canvas& gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderH, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("TFT Display", kMargin, 5, 2);
    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_YELLOW, TFT_NAVY);
    gfx.drawString("Factory", gfx.width() - kMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas& gfx)
{
    const int16_t y = gfx.height() - kFooterH;
    gfx.fillRect(0, y, gfx.width(), kFooterH, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(backFocused ? "BOOT=back" : "BOOT=next demo", kMargin, y + 4, 1);
    drawBackButton(gfx, backFocused);
}

template <typename Canvas>
void drawCard(Canvas& gfx, const int16_t x, const int16_t y, const int16_t w, const int16_t h,
              const char* label, const String& value, const uint16_t accent)
{
    gfx.fillRoundRect(x, y, w, h, 6, kUiCard);
    gfx.drawRoundRect(x, y, w, h, 6, kUiEdge);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(kUiMuted, kUiCard);
    gfx.drawString(label, x + 8, y + 5, 1);
    gfx.setTextColor(accent, kUiCard);
    gfx.drawString(value, x + 8, y + 18, 2);
}

template <typename Canvas>
void drawSummary(Canvas& gfx)
{
    drawCard(gfx, 8, 36, 96, 42, "Width", String(gfx.width()), TFT_CYAN);
    drawCard(gfx, 112, 36, 96, 42, "Height", String(gfx.height()), TFT_CYAN);
    drawCard(gfx, 216, 36, 96, 42, "Rotation", currentRotationLabel(), TFT_YELLOW);
    gfx.setTextColor(TFT_WHITE, kUiBg);
    gfx.drawString("Display online. Patterns auto-cycle.", 8, 104, 2);
}

template <typename Canvas>
void drawColorBars(Canvas& gfx)
{
    const uint16_t colors[] = {
        TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN,
        TFT_MAGENTA, TFT_YELLOW, TFT_WHITE, TFT_BLACK
    };
    const int16_t top = 30;
    const int16_t bottom = gfx.height() - kFooterH - 2;
    const int16_t barH = bottom - top;
    const int16_t barW = gfx.width() / 8;
    for (uint8_t i = 0; i < 8; ++i) {
        gfx.fillRect(i * barW, top, barW + 1, barH, colors[i]);
    }
}

template <typename Canvas>
void drawGeometry(Canvas& gfx)
{
    for (int16_t i = 0; i < 10; ++i) {
        gfx.drawRoundRect(12 + i * 9, 36 + i * 5, 296 - i * 18, 112 - i * 10, 8,
                          gfx.color565(20 * i, 255 - 18 * i, 80 + 12 * i));
    }
    gfx.fillCircle(gfx.width() / 2, 92, 26, TFT_ORANGE);
    gfx.drawLine(8, 32, gfx.width() - 8, 150, TFT_WHITE);
    gfx.drawLine(gfx.width() - 8, 32, 8, 150, TFT_WHITE);
}

template <typename Canvas>
void drawTextDemo(Canvas& gfx)
{
    gfx.setTextColor(TFT_CYAN, kUiBg);
    gfx.drawString("Font 1 factory text", 12, 42, 1);
    gfx.setTextColor(TFT_YELLOW, kUiBg);
    gfx.drawString("Font 2 factory text", 12, 62, 2);
    gfx.setTextColor(TFT_GREEN, kUiBg);
    gfx.drawString("1234567890", 12, 92, 4);
    gfx.setTextColor(TFT_WHITE, kUiBg);
    gfx.drawCentreString("CENTER", gfx.width() / 2, 138, 2);
}

template <typename Canvas>
void drawUi(Canvas& gfx)
{
    gfx.fillScreen(kUiBg);
    drawHeader(gfx);
    drawFooter(gfx);

    switch (demo) {
        case Demo::Summary:
            drawSummary(gfx);
            break;

        case Demo::ColorBars:
            drawColorBars(gfx);
            break;

        case Demo::Geometry:
            drawGeometry(gfx);
            break;

        case Demo::Text:
            drawTextDemo(gfx);
            break;

        case Demo::kCount:
        default:
            break;
    }
}
}  // namespace

void init()
{
    demo = Demo::Summary;
    backFocused = false;
    screenDirty = true;
    lastDrawMs = 0;
    lastAdvanceMs = millis();
    encSnapshot = g.encRaw;
}

void update()
{
    if (updateBinaryBackFocus(encSnapshot, backFocused)) {
        screenDirty = true;
    }

    (void)takeUserButton();

    if (takeEncoderButton()) {
        if (backFocused) {
            requestExitSubPage();
            return;
        }
        nextDemo();
    }

    if ((millis() - lastAdvanceMs) > 2500U) {
        nextDemo();
    }
}

void render()
{
    const uint32_t now = millis();
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < kFrameMs)) {
        return;
    }

    board_prepare_display();
    drawUi(tft);
    board_spi_deselect_all();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit() {}

}  // namespace page_tft
