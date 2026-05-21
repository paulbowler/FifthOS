#pragma once

#include <Arduino.h>

void gfx_init();
int16_t gfx_width();
int16_t gfx_height();
void gfx_clear(uint16_t color);
void gfx_pixel(int16_t x, int16_t y, uint16_t color);
void gfx_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void gfx_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_text(int16_t x, int16_t y, const char* text, size_t len, uint16_t color);
int16_t gfx_text_width(const char* text, size_t len);
void gfx_set_rotation(uint8_t rotation);
uint8_t gfx_rotation();
void gfx_begin_frame();
void gfx_end_frame();

// Internal helpers for retained widgets.
void gfx_set_text_scale(uint8_t scale);
uint8_t gfx_text_scale();
