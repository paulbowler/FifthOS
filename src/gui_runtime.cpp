#include "gui_runtime.h"

#include <Wire.h>

#include "forth_runtime.h"
#include "gfx_backend.h"
#include "vm.h"

namespace {

constexpr uint8_t TOUCH_ADDR = 0x38;
constexpr int TOUCH_SCL = 39;
constexpr int TOUCH_SDA = 40;
constexpr int TOUCH_INT = 41;

constexpr int MAX_STYLES = 16;
constexpr int MAX_NODES = 64;
constexpr int MAX_APPS = 4;
constexpr uint32_t TIMER_PERIOD_MS = 250;
constexpr int SWIPE_THRESHOLD = 48;
constexpr int TAP_THRESHOLD = 16;

enum StyleField : int16_t {
    STYLE_FG = 0,
    STYLE_BG = 1,
    STYLE_BORDER = 2,
    STYLE_PADDING = 3,
    STYLE_FONT = 4,
};

enum NodeField : int16_t {
    NODE_PARENT = 0,
    NODE_FIRST_CHILD = 1,
    NODE_NEXT_SIBLING = 2,
    NODE_X = 3,
    NODE_Y = 4,
    NODE_W = 5,
    NODE_H = 6,
    NODE_FLAGS = 7,
    NODE_DRAW_XT = 8,
    NODE_EVENT_XT = 9,
    NODE_STYLE = 10,
    NODE_USER = 11,
    NODE_KIND = 12,
    NODE_TEXT_ADDR = 13,
    NODE_TEXT_LEN = 14,
    NODE_BIND_ADDR = 15,
    NODE_MIN = 16,
    NODE_MAX = 17,
    NODE_POINTS_ADDR = 18,
    NODE_POINTS_COUNT = 19,
};

struct Style {
    bool used;
    uint16_t fg;
    uint16_t bg;
    uint16_t border;
    uint16_t padding;
    uint16_t font;
};

struct Node {
    bool used;
    int16_t parent;
    int16_t firstChild;
    int16_t nextSibling;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint16_t flags;
    long drawXt;
    long eventXt;
    int16_t style;
    long user;
    uint8_t kind;
    long textAddr;
    long textLen;
    long bindAddr;
    long minValue;
    long maxValue;
    long pointsAddr;
    long pointsCount;
};

struct App {
    bool used;
    int16_t screens[8];
    int16_t count;
    int16_t active;
};

Style styles[MAX_STYLES] = {};
Node nodes[MAX_NODES] = {};
App apps[MAX_APPS] = {};
int16_t activeApp = 0;

bool touchReady = false;
bool touchPressed = false;
int16_t touchX = 0;
int16_t touchY = 0;
int16_t lastTouchX = 0;
int16_t lastTouchY = 0;
int16_t touchStartX = 0;
int16_t touchStartY = 0;
uint32_t touchStartTick = 0;
uint32_t lastTimerTick = 0;
FifthGuiEvent currentEvent = { EVENT_NONE, 0, 0, 0, 0, 0, 0 };

bool i2cRead(uint8_t reg, uint8_t* data, size_t len)
{
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    size_t readLen = Wire.requestFrom(static_cast<int>(TOUCH_ADDR), static_cast<int>(len));
    if (readLen != len) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        data[i] = Wire.read();
    }
    return true;
}

bool touchReadPoint(int16_t* x, int16_t* y)
{
    uint8_t points = 0;
    if (!i2cRead(0x02, &points, 1)) {
        return false;
    }
    if ((points & 0x0F) == 0) {
        return false;
    }

    uint8_t raw[4] = {};
    if (!i2cRead(0x03, raw, sizeof(raw))) {
        return false;
    }

    int16_t rx = static_cast<int16_t>(((raw[0] & 0x0F) << 8) | raw[1]);
    int16_t ry = static_cast<int16_t>(((raw[2] & 0x0F) << 8) | raw[3]);

    switch (gfx_rotation()) {
        case 1:
            *x = gfx_width() - 1 - ry;
            *y = rx;
            break;
        case 2:
            *x = gfx_width() - 1 - rx;
            *y = gfx_height() - 1 - ry;
            break;
        case 3:
            *x = ry;
            *y = gfx_height() - 1 - rx;
            break;
        case 0:
        default:
            *x = rx;
            *y = ry;
            break;
    }
    return true;
}

char* forthChars(long addr)
{
    return reinterpret_cast<char*>(cData + addr);
}

long forthCell(long addr)
{
    return data[addr >> 2];
}

Style* stylePtr(int16_t id)
{
    if (id <= 0 || id >= MAX_STYLES || !styles[id].used) {
        return nullptr;
    }
    return &styles[id];
}

Node* nodePtr(int16_t id)
{
    if (id <= 0 || id >= MAX_NODES || !nodes[id].used) {
        return nullptr;
    }
    return &nodes[id];
}

App* appPtr(int16_t id)
{
    if (id <= 0 || id >= MAX_APPS || !apps[id].used) {
        return nullptr;
    }
    return &apps[id];
}

Style fallbackStyle()
{
    return { true, 0xFFFF, 0x0000, 0x7BEF, 4, 1 };
}

const Style& nodeStyle(const Node& node)
{
    Style* style = stylePtr(node.style);
    if (style) {
        return *style;
    }
    static Style fallback = fallbackStyle();
    return fallback;
}

void nodeAbsoluteXY(int16_t id, int16_t* outX, int16_t* outY)
{
    Node* node = nodePtr(id);
    if (!node) {
        *outX = 0;
        *outY = 0;
        return;
    }

    int16_t x = node->x;
    int16_t y = node->y;
    int16_t parent = node->parent;
    while (parent > 0) {
        Node* p = nodePtr(parent);
        if (!p) {
            break;
        }
        x += p->x;
        y += p->y;
        parent = p->parent;
    }
    *outX = x;
    *outY = y;
}

bool pointInNode(int16_t id, int16_t x, int16_t y)
{
    Node* node = nodePtr(id);
    if (!node) {
        return false;
    }
    int16_t sx = 0;
    int16_t sy = 0;
    nodeAbsoluteXY(id, &sx, &sy);
    return x >= sx && y >= sy && x < sx + node->w && y < sy + node->h;
}

void callForthXt(long xt, long a = 0, long b = 0, long c = 0, long d = 0, long e = 0, bool useA = false, bool useB = false, bool useC = false, bool useD = false, bool useE = false)
{
    if (xt == 0) {
        return;
    }

    char buffer[160];
    size_t offset = 0;
    if (useA) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%ld ", a);
    }
    if (useB) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%ld ", b);
    }
    if (useC) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%ld ", c);
    }
    if (useD) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%ld ", d);
    }
    if (useE) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%ld ", e);
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "%ld EXECUTE", xt);
    forth_eval_text(buffer);
}

void drawCenteredText(int16_t x, int16_t y, int16_t w, const char* text, size_t len, uint16_t color)
{
    int16_t textW = gfx_text_width(text, len);
    int16_t tx = x + ((w - textW) / 2);
    if (tx < x) {
        tx = x;
    }
    gfx_text(tx, y, text, len, color);
}

void drawRightText(int16_t x, int16_t y, int16_t w, const char* text, size_t len, uint16_t color)
{
    int16_t textW = gfx_text_width(text, len);
    int16_t tx = x + w - textW;
    if (tx < x) {
        tx = x;
    }
    gfx_text(tx, y, text, len, color);
}

void drawNodeBuiltin(int16_t id)
{
    Node* node = nodePtr(id);
    if (!node || !(node->flags & NODE_FLAG_VISIBLE)) {
        return;
    }

    const Style& style = nodeStyle(*node);
    int16_t sx = 0;
    int16_t sy = 0;
    nodeAbsoluteXY(id, &sx, &sy);

    uint8_t scale = style.font == 0 ? 1 : static_cast<uint8_t>(style.font);
    gfx_set_text_scale(scale);

    switch (node->kind) {
        case WIDGET_PANEL:
            gfx_fill_rect(sx, sy, node->w, node->h, style.bg);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            break;
        case WIDGET_LABEL:
            gfx_text(sx + style.padding, sy + style.padding, forthChars(node->textAddr), node->textLen, style.fg);
            break;
        case WIDGET_STATUS_BAR:
            gfx_fill_rect(sx, sy, node->w, node->h, style.bg);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            drawCenteredText(sx + style.padding, sy + style.padding, node->w - (style.padding * 2), forthChars(node->textAddr), node->textLen, style.fg);
            break;
        case WIDGET_BUTTON:
            gfx_fill_rect(sx, sy, node->w, node->h, style.bg);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            drawCenteredText(sx + style.padding, sy + style.padding, node->w - (style.padding * 2), forthChars(node->textAddr), node->textLen, style.fg);
            break;
        case WIDGET_VALUE: {
            gfx_fill_rect(sx, sy, node->w, node->h, style.bg);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            gfx_text(sx + style.padding, sy + style.padding, forthChars(node->textAddr), node->textLen, style.fg);
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%ld", forthCell(node->bindAddr));
            drawRightText(sx + style.padding, sy + style.padding, node->w - (style.padding * 2), buffer, strlen(buffer), style.fg);
            break;
        }
        case WIDGET_GAUGE_V: {
            gfx_fill_rect(sx, sy, node->w, node->h, style.bg);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            long value = forthCell(node->bindAddr);
            long span = node->maxValue - node->minValue;
            if (span <= 0) {
                span = 1;
            }
            if (value < node->minValue) {
                value = node->minValue;
            }
            if (value > node->maxValue) {
                value = node->maxValue;
            }
            int16_t innerW = node->w - (style.padding * 2);
            int16_t innerH = node->h - (style.padding * 2);
            int16_t fillH = static_cast<int16_t>((innerH * (value - node->minValue)) / span);
            gfx_fill_rect(sx + style.padding, sy + style.padding + (innerH - fillH), innerW, fillH, style.fg);
            break;
        }
        case WIDGET_CHART: {
            gfx_fill_rect(sx, sy, node->w, node->h, style.bg);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            long count = node->pointsCount;
            if (count < 2) {
                break;
            }
            long span = node->maxValue - node->minValue;
            if (span <= 0) {
                span = 1;
            }
            int16_t innerX = sx + style.padding;
            int16_t innerY = sy + style.padding;
            int16_t innerW = node->w - (style.padding * 2);
            int16_t innerH = node->h - (style.padding * 2);
            for (long i = 1; i < count; i++) {
                long a = forthCell(node->pointsAddr + ((i - 1) * 4));
                long b = forthCell(node->pointsAddr + (i * 4));
                int16_t x1 = innerX + static_cast<int16_t>((innerW * (i - 1)) / (count - 1));
                int16_t x2 = innerX + static_cast<int16_t>((innerW * i) / (count - 1));
                int16_t y1 = innerY + innerH - static_cast<int16_t>((innerH * (a - node->minValue)) / span);
                int16_t y2 = innerY + innerH - static_cast<int16_t>((innerH * (b - node->minValue)) / span);
                gfx_line(x1, y1, x2, y2, style.fg);
            }
            break;
        }
        case WIDGET_ALARM: {
            bool active = forthCell(node->bindAddr) != 0;
            uint16_t fill = active ? style.border : style.bg;
            gfx_fill_rect(sx, sy, node->w, node->h, fill);
            gfx_rect(sx, sy, node->w, node->h, style.border);
            drawCenteredText(sx + style.padding, sy + style.padding, node->w - (style.padding * 2), forthChars(node->textAddr), node->textLen, style.fg);
            break;
        }
        default:
            if (node->drawXt) {
                callForthXt(node->drawXt, id, 0, 0, 0, 0, true);
            }
            break;
    }
}

void drawNodeRecursive(int16_t id)
{
    Node* node = nodePtr(id);
    if (!node || !(node->flags & NODE_FLAG_VISIBLE)) {
        return;
    }

    drawNodeBuiltin(id);

    int16_t child = node->firstChild;
    while (child > 0) {
        drawNodeRecursive(child);
        Node* childNode = nodePtr(child);
        child = childNode ? childNode->nextSibling : 0;
    }

    node->flags &= ~NODE_FLAG_DIRTY;
}

bool appNeedsDraw(const App& app)
{
    if (app.active < 0) {
        return false;
    }

    int16_t root = app.screens[app.active];
    Node* rootNode = nodePtr(root);
    return rootNode && (rootNode->flags & NODE_FLAG_DIRTY);
}

int16_t deepestHit(int16_t id, int16_t x, int16_t y)
{
    Node* node = nodePtr(id);
    if (!node || !(node->flags & NODE_FLAG_VISIBLE)) {
        return 0;
    }

    int16_t child = node->firstChild;
    while (child > 0) {
        int16_t hit = deepestHit(child, x, y);
        if (hit > 0) {
            return hit;
        }
        Node* childNode = nodePtr(child);
        child = childNode ? childNode->nextSibling : 0;
    }

    if ((node->flags & NODE_FLAG_TOUCHABLE) && pointInNode(id, x, y)) {
        return id;
    }
    return 0;
}

void bubbleEventInternal(int16_t nodeId, uint8_t eventType)
{
    int16_t cursor = nodeId;
    while (cursor > 0) {
        Node* node = nodePtr(cursor);
        if (!node) {
            break;
        }
        if (node->eventXt) {
            callForthXt(node->eventXt,
                        cursor,
                        eventType,
                        currentEvent.x,
                        currentEvent.y,
                        currentEvent.dx,
                        true,
                        true,
                        true,
                        true,
                        true);
        }
        cursor = node->parent;
    }
}

void activateScreen(App& app, int16_t index)
{
    if (app.count <= 0) {
        app.active = -1;
        return;
    }

    if (index < 0) {
        index = app.count - 1;
    }
    if (index >= app.count) {
        index = 0;
    }

    app.active = index;
    for (int i = 0; i < app.count; i++) {
        Node* screen = nodePtr(app.screens[i]);
        if (!screen) {
            continue;
        }
        if (i == app.active) {
            screen->flags |= NODE_FLAG_VISIBLE | NODE_FLAG_DIRTY | NODE_FLAG_TOUCHABLE;
        } else {
            screen->flags &= ~NODE_FLAG_VISIBLE;
        }
    }
}

void dispatchTouchEvent(uint8_t type)
{
    currentEvent.type = type;
    currentEvent.target = 0;
    currentEvent.tick = millis();

    App* app = appPtr(activeApp);
    if (!app || app->active < 0) {
        return;
    }

    int16_t screen = app->screens[app->active];
    currentEvent.target = deepestHit(screen, currentEvent.x, currentEvent.y);

    if (currentEvent.target > 0) {
        bubbleEventInternal(currentEvent.target, type);
    }

    if (type == EVENT_TOUCH_SWIPE_LEFT) {
        gui_app_next(activeApp);
    } else if (type == EVENT_TOUCH_SWIPE_RIGHT) {
        gui_app_prev(activeApp);
    }
}

} // namespace

void gui_runtime_init()
{
    gfx_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    pinMode(TOUCH_INT, INPUT_PULLUP);
    touchReady = true;
    currentEvent = { EVENT_NONE, 0, 0, 0, 0, millis(), 0 };
}

void gui_runtime_tick()
{
    gui_touch_update();

    App* app = appPtr(activeApp);
    uint32_t now = millis();
    if (now - lastTimerTick >= TIMER_PERIOD_MS) {
        lastTimerTick = now;
        currentEvent.type = EVENT_TIMER;
        currentEvent.tick = now;
        if (app && app->active >= 0) {
            bubbleEventInternal(app->screens[app->active], EVENT_TIMER);
        }
    }

    if (app && appNeedsDraw(*app)) {
        gui_app_draw(activeApp);
    }
}

bool gui_touch_update()
{
    if (!touchReady) {
        return false;
    }

    int16_t x = 0;
    int16_t y = 0;
    bool pressed = touchReadPoint(&x, &y);
    uint32_t now = millis();

    if (pressed) {
        currentEvent.x = x;
        currentEvent.y = y;
        currentEvent.dx = x - lastTouchX;
        currentEvent.dy = y - lastTouchY;
        currentEvent.tick = now;

        if (!touchPressed) {
            touchPressed = true;
            touchStartX = x;
            touchStartY = y;
            touchStartTick = now;
            currentEvent.dx = 0;
            currentEvent.dy = 0;
            dispatchTouchEvent(EVENT_TOUCH_DOWN);
        } else if (x != lastTouchX || y != lastTouchY) {
            dispatchTouchEvent(EVENT_TOUCH_MOVE);
        }

        lastTouchX = x;
        lastTouchY = y;
        touchX = x;
        touchY = y;
        return true;
    }

    if (touchPressed) {
        touchPressed = false;
        currentEvent.x = lastTouchX;
        currentEvent.y = lastTouchY;
        currentEvent.dx = lastTouchX - touchStartX;
        currentEvent.dy = lastTouchY - touchStartY;
        currentEvent.tick = now;
        dispatchTouchEvent(EVENT_TOUCH_UP);

        int absDx = abs(currentEvent.dx);
        int absDy = abs(currentEvent.dy);
        if (absDx >= SWIPE_THRESHOLD && absDx > (absDy * 2)) {
            dispatchTouchEvent(currentEvent.dx > 0 ? EVENT_TOUCH_SWIPE_RIGHT : EVENT_TOUCH_SWIPE_LEFT);
        } else if (absDx <= TAP_THRESHOLD && absDy <= TAP_THRESHOLD) {
            dispatchTouchEvent(EVENT_TOUCH_TAP);
        }
        return true;
    }

    currentEvent.type = EVENT_NONE;
    return false;
}

uint8_t gui_swipe_detect()
{
    if (currentEvent.type == EVENT_TOUCH_SWIPE_LEFT || currentEvent.type == EVENT_TOUCH_SWIPE_RIGHT) {
        return currentEvent.type;
    }
    return EVENT_NONE;
}

const FifthGuiEvent& gui_current_event()
{
    return currentEvent;
}

int16_t gui_style_new()
{
    for (int16_t i = 1; i < MAX_STYLES; i++) {
        if (!styles[i].used) {
            styles[i] = fallbackStyle();
            styles[i].used = true;
            return i;
        }
    }
    return 0;
}

void gui_style_set(int16_t id, uint16_t fg, uint16_t bg, uint16_t border, uint16_t padding, uint16_t font)
{
    Style* style = stylePtr(id);
    if (!style) {
        return;
    }
    style->fg = fg;
    style->bg = bg;
    style->border = border;
    style->padding = padding;
    style->font = font == 0 ? 1 : font;
}

long gui_style_field_get(int16_t id, int16_t field)
{
    Style* style = stylePtr(id);
    if (!style) {
        return 0;
    }
    switch (field) {
        case STYLE_FG: return style->fg;
        case STYLE_BG: return style->bg;
        case STYLE_BORDER: return style->border;
        case STYLE_PADDING: return style->padding;
        case STYLE_FONT: return style->font;
        default: return 0;
    }
}

void gui_style_field_set(int16_t id, int16_t field, long value)
{
    Style* style = stylePtr(id);
    if (!style) {
        return;
    }
    switch (field) {
        case STYLE_FG: style->fg = static_cast<uint16_t>(value); break;
        case STYLE_BG: style->bg = static_cast<uint16_t>(value); break;
        case STYLE_BORDER: style->border = static_cast<uint16_t>(value); break;
        case STYLE_PADDING: style->padding = static_cast<uint16_t>(value); break;
        case STYLE_FONT: style->font = static_cast<uint16_t>(value == 0 ? 1 : value); break;
        default: break;
    }
}

int16_t gui_node_new()
{
    for (int16_t i = 1; i < MAX_NODES; i++) {
        if (!nodes[i].used) {
            nodes[i] = {};
            nodes[i].used = true;
            nodes[i].parent = 0;
            nodes[i].firstChild = 0;
            nodes[i].nextSibling = 0;
            nodes[i].flags = NODE_FLAG_VISIBLE | NODE_FLAG_DIRTY;
            return i;
        }
    }
    return 0;
}

void gui_node_add(int16_t parent, int16_t child)
{
    Node* p = nodePtr(parent);
    Node* c = nodePtr(child);
    if (!p || !c) {
        return;
    }

    gui_node_remove(child);
    c->parent = parent;
    c->nextSibling = 0;
    if (p->firstChild == 0) {
        p->firstChild = child;
    } else {
        int16_t cursor = p->firstChild;
        while (cursor > 0) {
            Node* next = nodePtr(cursor);
            if (!next || next->nextSibling == 0) {
                break;
            }
            cursor = next->nextSibling;
        }
        Node* tail = nodePtr(cursor);
        if (tail) {
            tail->nextSibling = child;
        }
    }
    gui_node_dirty(parent);
}

void gui_node_remove(int16_t child)
{
    Node* c = nodePtr(child);
    if (!c || c->parent == 0) {
        return;
    }
    Node* p = nodePtr(c->parent);
    if (!p) {
        c->parent = 0;
        return;
    }

    if (p->firstChild == child) {
        p->firstChild = c->nextSibling;
    } else {
        int16_t cursor = p->firstChild;
        while (cursor > 0) {
            Node* node = nodePtr(cursor);
            if (!node) {
                break;
            }
            if (node->nextSibling == child) {
                node->nextSibling = c->nextSibling;
                break;
            }
            cursor = node->nextSibling;
        }
    }
    c->parent = 0;
    c->nextSibling = 0;
    gui_node_dirty(p - nodes);
}

void gui_node_dirty(int16_t node)
{
    int16_t cursor = node;
    while (cursor > 0) {
        Node* n = nodePtr(cursor);
        if (!n) {
            break;
        }
        n->flags |= NODE_FLAG_DIRTY;
        cursor = n->parent;
    }
}

void gui_node_draw(int16_t node)
{
    drawNodeBuiltin(node);
}

void gui_node_draw_tree(int16_t node)
{
    drawNodeRecursive(node);
}

int16_t gui_node_hit(int16_t node, int16_t x, int16_t y)
{
    return deepestHit(node, x, y);
}

void gui_node_screen_xy(int16_t node, int16_t* x, int16_t* y)
{
    nodeAbsoluteXY(node, x, y);
}

void gui_node_set_bounds(int16_t node, int16_t x, int16_t y, int16_t w, int16_t h)
{
    Node* n = nodePtr(node);
    if (!n) {
        return;
    }
    n->x = x;
    n->y = y;
    n->w = w;
    n->h = h;
    gui_node_dirty(node);
}

bool gui_node_visible(int16_t node)
{
    Node* n = nodePtr(node);
    return n && (n->flags & NODE_FLAG_VISIBLE);
}

bool gui_node_touchable(int16_t node)
{
    Node* n = nodePtr(node);
    return n && (n->flags & NODE_FLAG_TOUCHABLE);
}

long gui_node_field_get(int16_t node, int16_t field)
{
    Node* n = nodePtr(node);
    if (!n) {
        return 0;
    }

    switch (field) {
        case NODE_PARENT: return n->parent;
        case NODE_FIRST_CHILD: return n->firstChild;
        case NODE_NEXT_SIBLING: return n->nextSibling;
        case NODE_X: return n->x;
        case NODE_Y: return n->y;
        case NODE_W: return n->w;
        case NODE_H: return n->h;
        case NODE_FLAGS: return n->flags;
        case NODE_DRAW_XT: return n->drawXt;
        case NODE_EVENT_XT: return n->eventXt;
        case NODE_STYLE: return n->style;
        case NODE_USER: return n->user;
        case NODE_KIND: return n->kind;
        case NODE_TEXT_ADDR: return n->textAddr;
        case NODE_TEXT_LEN: return n->textLen;
        case NODE_BIND_ADDR: return n->bindAddr;
        case NODE_MIN: return n->minValue;
        case NODE_MAX: return n->maxValue;
        case NODE_POINTS_ADDR: return n->pointsAddr;
        case NODE_POINTS_COUNT: return n->pointsCount;
        default: return 0;
    }
}

void gui_node_field_set(int16_t node, int16_t field, long value)
{
    Node* n = nodePtr(node);
    if (!n) {
        return;
    }

    switch (field) {
        case NODE_PARENT: n->parent = static_cast<int16_t>(value); break;
        case NODE_FIRST_CHILD: n->firstChild = static_cast<int16_t>(value); break;
        case NODE_NEXT_SIBLING: n->nextSibling = static_cast<int16_t>(value); break;
        case NODE_X: n->x = static_cast<int16_t>(value); break;
        case NODE_Y: n->y = static_cast<int16_t>(value); break;
        case NODE_W: n->w = static_cast<int16_t>(value); break;
        case NODE_H: n->h = static_cast<int16_t>(value); break;
        case NODE_FLAGS: n->flags = static_cast<uint16_t>(value); break;
        case NODE_DRAW_XT: n->drawXt = value; break;
        case NODE_EVENT_XT: n->eventXt = value; break;
        case NODE_STYLE: n->style = static_cast<int16_t>(value); break;
        case NODE_USER: n->user = value; break;
        case NODE_KIND: n->kind = static_cast<uint8_t>(value); break;
        case NODE_TEXT_ADDR: n->textAddr = value; break;
        case NODE_TEXT_LEN: n->textLen = value; break;
        case NODE_BIND_ADDR: n->bindAddr = value; break;
        case NODE_MIN: n->minValue = value; break;
        case NODE_MAX: n->maxValue = value; break;
        case NODE_POINTS_ADDR: n->pointsAddr = value; break;
        case NODE_POINTS_COUNT: n->pointsCount = value; break;
        default: break;
    }
    gui_node_dirty(node);
}

int16_t makeWidget(uint8_t kind, int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style)
{
    int16_t node = gui_node_new();
    if (node == 0) {
        return 0;
    }
    gui_node_set_bounds(node, x, y, w, h);
    gui_node_field_set(node, NODE_KIND, kind);
    gui_node_field_set(node, NODE_STYLE, style);
    gui_node_field_set(node, NODE_FLAGS, NODE_FLAG_VISIBLE | NODE_FLAG_DIRTY);
    if (parent > 0) {
        gui_node_add(parent, node);
    }
    return node;
}

int16_t gui_widget_panel(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style)
{
    return makeWidget(WIDGET_PANEL, parent, x, y, w, h, style);
}

int16_t gui_widget_label(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len)
{
    int16_t node = makeWidget(WIDGET_LABEL, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_TEXT_ADDR, addr);
    gui_node_field_set(node, NODE_TEXT_LEN, len);
    return node;
}

int16_t gui_widget_button(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len, long eventXt)
{
    int16_t node = makeWidget(WIDGET_BUTTON, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_TEXT_ADDR, addr);
    gui_node_field_set(node, NODE_TEXT_LEN, len);
    gui_node_field_set(node, NODE_EVENT_XT, eventXt);
    gui_node_field_set(node, NODE_FLAGS, NODE_FLAG_VISIBLE | NODE_FLAG_DIRTY | NODE_FLAG_TOUCHABLE | NODE_FLAG_FOCUSABLE);
    return node;
}

int16_t gui_widget_status(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len)
{
    int16_t node = makeWidget(WIDGET_STATUS_BAR, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_TEXT_ADDR, addr);
    gui_node_field_set(node, NODE_TEXT_LEN, len);
    return node;
}

int16_t gui_widget_value(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len, long valueAddr)
{
    int16_t node = makeWidget(WIDGET_VALUE, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_TEXT_ADDR, addr);
    gui_node_field_set(node, NODE_TEXT_LEN, len);
    gui_node_field_set(node, NODE_BIND_ADDR, valueAddr);
    return node;
}

int16_t gui_widget_gauge(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long valueAddr, long minValue, long maxValue)
{
    int16_t node = makeWidget(WIDGET_GAUGE_V, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_BIND_ADDR, valueAddr);
    gui_node_field_set(node, NODE_MIN, minValue);
    gui_node_field_set(node, NODE_MAX, maxValue);
    return node;
}

int16_t gui_widget_chart(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long pointsAddr, long count, long minValue, long maxValue)
{
    int16_t node = makeWidget(WIDGET_CHART, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_POINTS_ADDR, pointsAddr);
    gui_node_field_set(node, NODE_POINTS_COUNT, count);
    gui_node_field_set(node, NODE_MIN, minValue);
    gui_node_field_set(node, NODE_MAX, maxValue);
    return node;
}

int16_t gui_widget_alarm(int16_t parent, int16_t x, int16_t y, int16_t w, int16_t h, int16_t style, long addr, long len, long valueAddr)
{
    int16_t node = makeWidget(WIDGET_ALARM, parent, x, y, w, h, style);
    gui_node_field_set(node, NODE_TEXT_ADDR, addr);
    gui_node_field_set(node, NODE_TEXT_LEN, len);
    gui_node_field_set(node, NODE_BIND_ADDR, valueAddr);
    return node;
}

int16_t gui_app_new()
{
    for (int16_t i = 1; i < MAX_APPS; i++) {
        if (!apps[i].used) {
            apps[i] = {};
            apps[i].used = true;
            apps[i].active = -1;
            return i;
        }
    }
    return 0;
}

void gui_app_add_screen(int16_t appId, int16_t screen)
{
    App* app = appPtr(appId);
    if (!app || app->count >= 8) {
        return;
    }
    app->screens[app->count++] = screen;
    if (app->active < 0) {
        activateScreen(*app, 0);
    } else {
        Node* node = nodePtr(screen);
        if (node) {
            node->flags &= ~NODE_FLAG_VISIBLE;
        }
    }
}

void gui_app_next(int16_t appId)
{
    App* app = appPtr(appId);
    if (!app) {
        return;
    }
    activateScreen(*app, app->active + 1);
}

void gui_app_prev(int16_t appId)
{
    App* app = appPtr(appId);
    if (!app) {
        return;
    }
    activateScreen(*app, app->active - 1);
}

void gui_app_show(int16_t appId, int16_t screenIndex)
{
    App* app = appPtr(appId);
    if (!app) {
        return;
    }
    activeApp = appId;
    activateScreen(*app, screenIndex);
}

int16_t gui_app_active(int16_t appId)
{
    App* app = appPtr(appId);
    if (!app || app->active < 0) {
        return 0;
    }
    return app->screens[app->active];
}

void gui_app_draw(int16_t appId)
{
    App* app = appPtr(appId);
    if (!app || app->active < 0) {
        return;
    }
    int16_t root = app->screens[app->active];
    Node* rootNode = nodePtr(root);
    if (!rootNode) {
        return;
    }
    gfx_begin_frame();
    const Style& style = nodeStyle(*rootNode);
    gfx_clear(style.bg);
    drawNodeRecursive(root);
    gfx_end_frame();
}

bool gui_has_active_app()
{
    App* app = appPtr(activeApp);
    return app && app->active >= 0;
}

void gui_draw_active_app()
{
    if (activeApp > 0) {
        gui_app_draw(activeApp);
    }
}

void gui_event_dispatch(int16_t node, uint8_t eventType)
{
    currentEvent.type = eventType;
    currentEvent.target = node;
    bubbleEventInternal(node, eventType);
}

void gui_event_bubble(int16_t node, uint8_t eventType)
{
    gui_event_dispatch(node, eventType);
}
