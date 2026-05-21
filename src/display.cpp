#include <Arduino.h>

#include <Arduino_GFX_Library.h>

#include "display.h"

namespace {

constexpr int LCD_CS = 6;
constexpr int LCD_SCLK = 47;
constexpr int LCD_SDIO0 = 18;
constexpr int LCD_SDIO1 = 7;
constexpr int LCD_SDIO2 = 48;
constexpr int LCD_SDIO3 = 5;
constexpr int LCD_RESET = 17;
constexpr int LCD_ROTATION = 1;

Arduino_DataBus* bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);

bool displayReady = false;

} // namespace

Arduino_GFX* display = new Arduino_RM67162(bus, LCD_RESET, LCD_ROTATION);

void setupDisplay()
{
    displayReady = display->begin();
    if (!displayReady) {
        Serial.println("Display init failed");
        return;
    }

    display->fillScreen(BLACK);
    display->setTextWrap(false);
}

bool isDisplayReady()
{
    return displayReady;
}
