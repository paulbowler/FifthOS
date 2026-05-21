#pragma once

#include <Arduino.h>

enum FifthEventType : uint8_t {
    EVENT_NONE = 0,
    EVENT_TOUCH_DOWN = 1,
    EVENT_TOUCH_MOVE = 2,
    EVENT_TOUCH_UP = 3,
    EVENT_TOUCH_TAP = 4,
    EVENT_TOUCH_SWIPE_LEFT = 5,
    EVENT_TOUCH_SWIPE_RIGHT = 6,
    EVENT_TIMER = 7,
    EVENT_DRAW = 8,
};

enum FifthWidgetKind : uint8_t {
    WIDGET_NONE = 0,
    WIDGET_LABEL = 1,
    WIDGET_PANEL = 2,
    WIDGET_BUTTON = 3,
    WIDGET_STATUS_BAR = 4,
    WIDGET_VALUE = 5,
    WIDGET_GAUGE_V = 6,
    WIDGET_CHART = 7,
    WIDGET_ALARM = 8,
};

enum FifthNodeFlags : uint16_t {
    NODE_FLAG_VISIBLE = 1 << 0,
    NODE_FLAG_DIRTY = 1 << 1,
    NODE_FLAG_TOUCHABLE = 1 << 2,
    NODE_FLAG_FOCUSABLE = 1 << 3,
};

struct FifthGuiEvent {
    uint8_t type;
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    uint32_t tick;
    int16_t target;
};

void gui_runtime_init();
void gui_runtime_tick();

// Touch/event state
bool gui_touch_update();
uint8_t gui_swipe_detect();
const FifthGuiEvent& gui_current_event();

// Styles
int16_t gui_style_new();
void gui_style_set(int16_t id, uint16_t fg, uint16_t bg, uint16_t border, uint16_t padding, uint16_t font);
long gui_style_field_get(int16_t id, int16_t field);
void gui_style_field_set(int16_t id, int16_t field, long value);

// Nodes
int16_t gui_node_new();
void gui_node_add(int16_t parent, int16_t child);
void gui_node_remove(int16_t child);
void gui_node_dirty(int16_t node);
void gui_node_draw(int16_t node);
void gui_node_draw_tree(int16_t node);
int16_t gui_node_hit(int16_t node, int16_t x, int16_t y);
void gui_node_screen_xy(int16_t node, int16_t* x, int16_t* y);
void gui_node_set_bounds(int16_t node, int16_t x, int16_t y, int16_t w, int16_t h);
bool gui_node_visible(int16_t node);
bool gui_node_touchable(int16_t node);
long gui_node_field_get(int16_t node, int16_t field);
void gui_node_field_set(int16_t node, int16_t field, long value);

// Built-in widgets
int16_t gui_widget_panel(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style);
int16_t gui_widget_label(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len);
int16_t gui_widget_button(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len, long eventXt);
int16_t gui_widget_status(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len);
int16_t gui_widget_value(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len, long valueAddr);
int16_t gui_widget_gauge(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long valueAddr, long minValue, long maxValue);
int16_t gui_widget_chart(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long pointsAddr, long count, long minValue, long maxValue);
int16_t gui_widget_alarm(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len, long valueAddr);

// Apps
int16_t gui_app_new();
void gui_app_add_screen(int16_t app, int16_t screen);
void gui_app_next(int16_t app);
void gui_app_prev(int16_t app);
void gui_app_show(int16_t app, int16_t screenIndex);
int16_t gui_app_active(int16_t app);
void gui_app_draw(int16_t app);
bool gui_has_active_app();
void gui_draw_active_app();

// Events
void gui_event_dispatch(int16_t node, uint8_t eventType);
void gui_event_bubble(int16_t node, uint8_t eventType);
