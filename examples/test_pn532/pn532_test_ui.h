#pragma once

inline String statusDetailText()
{
    if (!detailLine.isEmpty()) {
        return detailLine;
    }

    switch (uiState) {
    case UiState::Init:
        return "Bringing up display and PN532 reader";
    case UiState::Scanning:
        return "Bring an ISO14443A tag close to the antenna";
    case UiState::CardFound:
        return "Tag identified successfully";
    case UiState::ReadFail:
        return "Tag event arrived, but payload read failed";
    case UiState::NoCard:
        return "Tag removed";
    case UiState::FatalError:
        return "Check serial output for details";
    }
    return "";
}

inline const char *footerText()
{
    switch (uiState) {
    case UiState::CardFound:
        return "Hold tag steady to keep this snapshot";
    case UiState::ReadFail:
        return "Retrying IRQ-driven scan after this event";
    case UiState::FatalError:
        return "Reader halted";
    case UiState::NoCard:
    case UiState::Scanning:
    case UiState::Init:
    default:
        return "Adafruit PN532 I2C + IRQ | espressif32 6.13.0";
    }
}

inline uint16_t statusFillColor()
{
    switch (uiState) {
    case UiState::CardFound:
        return kColorPassBg;
    case UiState::ReadFail:
        return kColorWarnBg;
    case UiState::FatalError:
        return kColorFailBg;
    case UiState::NoCard:
        return kColorCard;
    case UiState::Init:
    case UiState::Scanning:
    default:
        return kColorPanel;
    }
}

inline String uidText()
{
    return currentCard.uid.isEmpty() ? String("-") : currentCard.uid;
}

inline String typeText()
{
    return currentCard.type.isEmpty() ? String("-") : currentCard.type;
}

inline String uidLengthText()
{
    if (currentCard.uidLength == 0) {
        return "-";
    }
    return String(currentCard.uidLength) + " bytes";
}

inline String readerText()
{
    return readerVersionData == 0 ? String("-") : String("PN532 FW ") + firmwareVersionText();
}

inline String cardIdText()
{
    return currentCard.cardId.isEmpty() ? String("-") : currentCard.cardId;
}

inline String statusPillText()
{
    switch (uiState) {
    case UiState::Init:
        return "BOOT";
    case UiState::Scanning:
        return "SCAN";
    case UiState::CardFound:
        return "LIVE";
    case UiState::ReadFail:
        return "WARN";
    case UiState::NoCard:
        return "IDLE";
    case UiState::FatalError:
        return "STOP";
    }
    return "?";
}

template <typename Canvas>
void drawHeader(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), kHeaderHeight, TFT_NAVY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_NAVY);
    gfx.drawString("PN532 NFC Test", kUiMargin, 5, 2);

    gfx.setTextDatum(TR_DATUM);
    gfx.setTextColor(TFT_CYAN, TFT_NAVY);
    gfx.drawString("I2C / IRQ", gfx.width() - kUiMargin, 7, 1);
    gfx.setTextDatum(TL_DATUM);
}

template <typename Canvas>
void drawFooter(Canvas &gfx)
{
    const int16_t y = gfx.height() - kFooterHeight;
    gfx.fillRect(0, y, gfx.width(), kFooterHeight, TFT_DARKGREY);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_WHITE, TFT_DARKGREY);
    gfx.drawString(footerText(), 6, y + 4, 1);
}

template <typename Canvas>
void drawStatusPanel(Canvas &gfx)
{
    const uint16_t accent = stateColor();
    const uint16_t fill = statusFillColor();

    gfx.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, fill);
    gfx.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 8, accent);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(accent, fill);
    gfx.drawString(stateLabel(), kStatusX + 10, kStatusY + 6, 2);

    gfx.fillRoundRect(kStatusX + kStatusW - 60, kStatusY + 7, 48, 16, 6, accent);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_BLACK, accent);
    gfx.drawString(statusPillText(), kStatusX + kStatusW - 36, kStatusY + 15, 1);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_LIGHTGREY, fill);
    gfx.drawString(statusDetailText(), kStatusX + 10, kStatusY + 24, 1);
}

template <typename Canvas>
void drawFieldCard(Canvas &gfx, int16_t x, int16_t y, int16_t w, int16_t h,
                   const char *label, const String &value, uint16_t borderColor)
{
    gfx.fillRoundRect(x, y, w, h, 6, kColorCard);
    gfx.drawRoundRect(x, y, w, h, 6, borderColor);

    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_CYAN, kColorCard);
    gfx.drawString(label, x + 8, y + 5, 1);

    gfx.setTextColor(TFT_WHITE, kColorCard);
    gfx.drawString(value, x + 56, y + 5, 1);
}

template <typename Canvas>
void drawCardDetails(Canvas &gfx)
{
    gfx.fillRoundRect(kUidX, kUidY, kUidW, kUidH, 6, kColorCard);
    gfx.drawRoundRect(kUidX, kUidY, kUidW, kUidH, 6, stateColor());
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(TFT_CYAN, kColorCard);
    gfx.drawString("UID", kUidX + 8, kUidY + 6, 1);
    gfx.setTextColor(TFT_WHITE, kColorCard);
    gfx.drawString(uidText(), kUidX + 56, kUidY + 6, 1);

    drawFieldCard(gfx, kMetaLeftX, kMetaY, kMetaW, kMetaH, "Type", typeText(), kColorPanelEdge);
    drawFieldCard(gfx, kMetaRightX, kMetaY, kMetaW, kMetaH, "UID Len",
                  uidLengthText(), kColorPanelEdge);
    drawFieldCard(gfx, kSizeX, kSizeY, kMetaW, kSizeH, "Reader", readerText(),
                  kColorPanelEdge);
    drawFieldCard(gfx, kMetaRightX, kSizeY, kMetaW, kSizeH, "Card ID", cardIdText(),
                  currentCard.uidLength == 4 ? kColorPanelEdge : TFT_DARKGREY);
}

template <typename Canvas>
void drawUi(Canvas &gfx)
{
    gfx.fillRect(0, 0, gfx.width(), gfx.height(), kColorBg);
    drawHeader(gfx);
    drawStatusPanel(gfx);
    drawCardDetails(gfx);
    drawFooter(gfx);
}
