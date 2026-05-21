#include "gfx_backend.h"

#include <Arduino_GFX_Library.h>

#include "display.h"

namespace {

uint8_t currentRotation = 1;
uint8_t currentTextScale = 1;

void applyTextScale()
{
    if (isDisplayReady()) {
        display->setTextSize(currentTextScale);
    }
}

} // namespace

void gfx_init()
{
    if (isDisplayReady()) {
        gfx_set_rotation(currentRotation);
        gfx_set_text_scale(currentTextScale);
        return;
    }

    setupDisplay();
    if (!isDisplayReady()) {
        return;
    }
    gfx_set_rotation(currentRotation);
    gfx_set_text_scale(currentTextScale);
    display->setTextWrap(false);
}

int16_t gfx_width()
{
    return isDisplayReady() ? display->width() : 0;
}

int16_t gfx_height()
{
    return isDisplayReady() ? display->height() : 0;
}

void gfx_clear(uint16_t color)
{
    if (isDisplayReady()) {
        display->fillScreen(color);
    }
}

void gfx_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (isDisplayReady()) {
        display->drawPixel(x, y, color);
    }
}

void gfx_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    if (isDisplayReady()) {
        display->drawLine(x1, y1, x2, y2, color);
    }
}

void gfx_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (isDisplayReady()) {
        display->drawRect(x, y, w, h, color);
    }
}

void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (isDisplayReady()) {
        display->fillRect(x, y, w, h, color);
    }
}

void gfx_text(int16_t x, int16_t y, const char* text, size_t len, uint16_t color)
{
    if (!isDisplayReady()) {
        return;
    }

    display->setCursor(x, y);
    display->setTextColor(color);
    for (size_t i = 0; i < len; i++) {
        display->write(static_cast<uint8_t>(text[i]));
    }
}

int16_t gfx_text_width(const char* text, size_t len)
{
    if (!isDisplayReady()) {
        return 0;
    }

    String s;
    s.reserve(len);
    for (size_t i = 0; i < len; i++) {
        s += text[i];
    }
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    display->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
    return static_cast<int16_t>(w);
}

void gfx_set_rotation(uint8_t rotation)
{
    currentRotation = rotation & 0x3;
    if (isDisplayReady()) {
        display->setRotation(currentRotation);
    }
}

uint8_t gfx_rotation()
{
    return currentRotation;
}

void gfx_begin_frame()
{
}

void gfx_end_frame()
{
}

void gfx_set_text_scale(uint8_t scale)
{
    currentTextScale = scale == 0 ? 1 : scale;
    applyTextScale();
}

uint8_t gfx_text_scale()
{
    return currentTextScale;
}
