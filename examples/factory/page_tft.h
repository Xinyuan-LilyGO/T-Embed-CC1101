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
    if (!screenDirty || (lastDrawMs != 0 && (now - lastDrawMs) < 80U)) {
        return;
    }

    board_prepare_display();
    gSubPageGfx.beginFrame();
    tft.fillScreen(kUiBg);
    drawPageHeader("TFT Display", TFT_YELLOW);
    drawPageFooter("BOOT=next demo", backFocused);

    switch (demo) {
        case Demo::Summary:
            drawCard(8, 36, 96, 42, "Width", String(tft.width()), TFT_CYAN);
            drawCard(112, 36, 96, 42, "Height", String(tft.height()), TFT_CYAN);
            drawCard(216, 36, 96, 42, "Rotation", currentRotationLabel(), TFT_YELLOW);
            tft.setTextColor(TFT_WHITE, kUiBg);
            tft.drawString("Display online. Patterns auto-cycle.", 8, 104, 2);
            break;

        case Demo::ColorBars: {
            const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, TFT_WHITE, TFT_BLACK};
            const int16_t barW = tft.width() / 8;
            for (uint8_t i = 0; i < 8; ++i) {
                tft.fillRect(i * barW, 30, barW + 1, 120, colors[i]);
            }
            break;
        }

        case Demo::Geometry:
            for (int16_t i = 0; i < 10; ++i) {
                tft.drawRoundRect(12 + i * 9, 36 + i * 5, 296 - i * 18, 112 - i * 10, 8,
                                  tft.color565(20 * i, 255 - 18 * i, 80 + 12 * i));
            }
            tft.fillCircle(160, 92, 26, TFT_ORANGE);
            tft.drawLine(8, 32, 312, 150, TFT_WHITE);
            tft.drawLine(312, 32, 8, 150, TFT_WHITE);
            break;

        case Demo::Text:
            tft.setTextColor(TFT_CYAN, kUiBg);
            tft.drawString("Font 1 factory text", 12, 42, 1);
            tft.setTextColor(TFT_YELLOW, kUiBg);
            tft.drawString("Font 2 factory text", 12, 62, 2);
            tft.setTextColor(TFT_GREEN, kUiBg);
            tft.drawString("1234567890", 12, 92, 4);
            tft.setTextColor(TFT_WHITE, kUiBg);
            tft.drawCentreString("CENTER", 160, 138, 2);
            break;

        case Demo::kCount:
        default:
            break;
    }

    board_spi_deselect_all();
    gSubPageGfx.endFrame();
    screenDirty = false;
    lastDrawMs = now;
}

void deinit() {}

}  // namespace page_tft
