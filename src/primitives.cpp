#include <Arduino.h>
#include "primitives.h"
#include "globals.h"
#include "data_runtime.h"
#include "gfx_backend.h"
#include "gui_runtime.h"
#include "network.h"
#include "vm.h"
#include "common.h"
#include "http.h"

/******************************************************************************/
/* PRIMITIVES                                                                 */
/******************************************************************************/

namespace {

constexpr long FORTH_ADDR_IN = 0x198;
constexpr long FORTH_ADDR_NTIB = 0x19C;
constexpr long FORTH_ADDR_TTIB = 0x1A0;
constexpr long FORTH_ADDR_CONTEXT = 0x1A8;
constexpr long FORTH_ADDR_CP = 0x1AC;
constexpr long FORTH_ADDR_LAST = 0x1B0;
constexpr uint8_t FORTH_IMMEDIATE = 0x80;
constexpr uint8_t MAX_LOCALS = 16;
constexpr uint8_t MAX_LOCAL_NAME = 16;
char localsCompileNames[MAX_LOCALS][MAX_LOCAL_NAME + 1] = {};

long forthVar(long addr)
{
    return data[addr >> 2];
}

void forthSetVar(long addr, long value)
{
    data[addr >> 2] = value;
}

long forthCurrentCp()
{
    return forthVar(FORTH_ADDR_CP);
}

void forthSetCurrentCp(long addr)
{
    forthSetVar(FORTH_ADDR_CP, addr);
}

long alignCell(long addr)
{
    return (addr + 3) & ~0x3L;
}

uint8_t countedLen(long addr)
{
    return static_cast<uint8_t>(cData[addr] & 0x1F);
}

String countedToken(long addr)
{
    const uint8_t len = countedLen(addr);
    String token;
    token.reserve(len);
    const char* text = reinterpret_cast<const char*>(cData + addr + 1);
    for (uint8_t i = 0; i < len; ++i) {
        token += text[i];
    }
    return token;
}

bool tokenEquals(long addr, const char* text)
{
    const uint8_t len = countedLen(addr);
    if (strlen(text) != len) {
        return false;
    }
    for (uint8_t i = 0; i < len; ++i) {
        if (cData[addr + 1 + i] != text[i]) {
            return false;
        }
    }
    return true;
}

void compileCell(long value)
{
    long cp = forthCurrentCp();
    data[cp >> 2] = value;
    forthSetCurrentCp(cp + 4);
}

bool parseNextInputToken(String& out)
{
    long in = forthVar(FORTH_ADDR_IN);
    const long ntib = forthVar(FORTH_ADDR_NTIB);
    const long tib = forthVar(FORTH_ADDR_TTIB);
    while (in < ntib && cData[tib + in] <= ' ') {
        ++in;
    }
    if (in >= ntib) {
        forthSetVar(FORTH_ADDR_IN, in);
        return false;
    }
    const long start = in;
    while (in < ntib && cData[tib + in] > ' ') {
        ++in;
    }
    forthSetVar(FORTH_ADDR_IN, in);
    out.remove(0);
    out.reserve(static_cast<unsigned int>(in - start));
    for (long i = start; i < in; ++i) {
        out += static_cast<char>(cData[tib + i]);
    }
    return true;
}

void endLocalCompileScope()
{
    if (!forthLocalsCompileActive) {
        return;
    }
    forthSetVar(FORTH_ADDR_LAST, forthLocalsSavedLast);
    forthSetVar(FORTH_ADDR_CONTEXT, forthLocalsSavedContext);
    forthLocalsCompileActive = false;
    for (uint8_t i = 0; i < MAX_LOCALS; ++i) {
        localsCompileNames[i][0] = '\0';
    }
    forthLocalsCompileCount = 0;
    forthLocalsCompileInputCount = 0;
}

bool tokenMatchesLocal(long tokenAddr, uint8_t index)
{
    const uint8_t len = countedLen(tokenAddr);
    const char* localName = localsCompileNames[index];
    const size_t localLen = strlen(localName);
    if (localLen != len) {
        return false;
    }
    for (uint8_t i = 0; i < len; ++i) {
        char a = cData[tokenAddr + 1 + i];
        char b = localName[i];
        if (a >= 'a' && a <= 'z') {
            a = static_cast<char>(a - 32);
        }
        if (b >= 'a' && b <= 'z') {
            b = static_cast<char>(b - 32);
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

String forthString(long addr, long len)
{
    String out;
    if (len <= 0) {
        return out;
    }
    out.reserve(static_cast<unsigned int>(len));
    const char* text = reinterpret_cast<const char*>(cData + addr);
    for (long i = 0; i < len; ++i) {
        out += text[i];
    }
    return out;
}

void replPrint(const String& text)
{
    Serial.print(text);
    HTTPout += text;
}

void replPrintLine(const String& text = String())
{
    replPrint(text);
    replPrint("\r\n");
}

void wifiPrintUrls()
{
    replPrintLine(String("REPL URL: ") + networkReplUrl());
    replPrintLine(String("WiFi setup URL: ") + networkSetupUrl());
}

}

void next(void)
{
    P = data[IP >> 2];
    IP += 4;
    WP = P + 4;
}

void accep()
/* WiFiClient */
{
    while (Serial.available()) {
        len = Serial.readBytes(cData, top);
    }
    Serial.write(cData, len);
    top = len;
}
void qrx(void)
{
    while (Serial.available() == 0) { };
    vmPush Serial.read();
    vmPush - 1;
}

void txsto(void)
{
    Serial.write((unsigned char)top);
    char c = top;
    HTTPout += c;
    vmPop;
}

void docon(void)
{
    vmPush data[WP >> 2];
}

void dolit(void)
{
    vmPush data[IP >> 2];
    IP += 4;
    next();
}

void dolist(void)
{
    if (forthCallDepth < 31) {
        ++forthCallDepth;
    }
    forthLocalFrames[forthCallDepth].active = false;
    forthLocalFrames[forthCallDepth].count = 0;
    forthLocalFrames[forthCallDepth].returnSlot = R + 1;
    rack[(unsigned char)++R] = IP;
    IP = WP;
    next();
}

void exitt(void)
{
    if (forthLocalFrames[forthCallDepth].active) {
        R = forthLocalFrames[forthCallDepth].returnSlot;
        forthLocalFrames[forthCallDepth].active = false;
        forthLocalFrames[forthCallDepth].count = 0;
    }
    IP = (long)rack[(unsigned char)R--];
    if (forthCallDepth > 0) {
        --forthCallDepth;
    }
    next();
}

void execu(void)
{
    P = top;
    WP = P + 4;
    vmPop;
}

void donext(void)
{
    if (rack[(unsigned char)R]) {
        rack[(unsigned char)R] -= 1;
        IP = data[IP >> 2];
    } else {
        IP += 4;
        (unsigned char)R--;
    }
    next();
}

void qbran(void)
{
    if (top == 0)
        IP = data[IP >> 2];
    else
        IP += 4;
    vmPop;
    next();
}

void bran(void)
{
    IP = data[IP >> 2];
    next();
}

void store(void)
{
    data[top >> 2] = stack[(unsigned char)S--];
    vmPop;
}

void at(void)
{
    top = data[top >> 2];
}

void cstor(void)
{
    cData[top] = (unsigned char)stack[(unsigned char)S--];
    vmPop;
}

void cat(void)
{
    top = (long)cData[top];
}

void rpat(void) { }
void rpsto(void) { }

void rfrom(void)
{
    vmPush rack[(unsigned char)R--];
}

void rat(void)
{
    vmPush rack[(unsigned char)R];
}

void tor(void)
{
    rack[(unsigned char)++R] = top;
    vmPop;
}

void spat(void) { }
void spsto(void) { }

void drop(void)
{
    vmPop;
}

void dup(void)
{
    stack[(unsigned char)++S] = top;
}

void swap(void)
{
    WP = top;
    top = stack[(unsigned char)S];
    stack[(unsigned char)S] = WP;
}

void over(void)
{
    vmPush stack[(unsigned char)(S - 1)];
}

void zless(void)
{
    top = (top < 0) LOGICAL;
}

void andd(void)
{
    top &= stack[(unsigned char)S--];
}

void orr(void)
{
    top |= stack[(unsigned char)S--];
}

void xorr(void)
{
    top ^= stack[(unsigned char)S--];
}

void uplus(void)
{
    stack[(unsigned char)S] += top;
    top = LOWER(stack[(unsigned char)S], top);
}

void nop(void)
{
    next();
}

void qdup(void)
{
    if (top)
        stack[(unsigned char)++S] = top;
}

void rot(void)
{
    WP = stack[(unsigned char)(S - 1)];
    stack[(unsigned char)(S - 1)] = stack[(unsigned char)S];
    stack[(unsigned char)S] = top;
    top = WP;
}

void ddrop(void)
{
    drop();
    drop();
}

void ddup(void)
{
    over();
    over();
}

void plus(void)
{
    top += stack[(unsigned char)S--];
}

void inver(void)
{
    top = -top - 1;
}

void negat(void)
{
    top = 0 - top;
}

void dnega(void)
{
    inver();
    tor();
    inver();
    vmPush 1;
    uplus();
    rfrom();
    plus();
}

void subb(void)
{
    top = stack[(unsigned char)S--] - top;
}

void abss(void)
{
    if (top < 0)
        top = -top;
}

void great(void)
{
    top = (stack[(unsigned char)S--] > top) LOGICAL;
}

void less(void)
{
    top = (stack[(unsigned char)S--] < top) LOGICAL;
}

void equal(void)
{
    top = (stack[(unsigned char)S--] == top) LOGICAL;
}

void uless(void)
{
    top = LOWER(stack[(unsigned char)S], top) LOGICAL;
    S--;
}

void ummod(void)
{
    d = (long long int)((unsigned long)top);
    m = (long long int)((unsigned long)stack[(unsigned char)S]);
    n = (long long int)((unsigned long)stack[(unsigned char)(S - 1)]);
    n += m << 32;
    vmPop;
    top = (unsigned long)(n / d);
    stack[(unsigned char)S] = (unsigned long)(n % d);
}
void msmod(void)
{
    d = (signed long long int)((signed long)top);
    m = (signed long long int)((signed long)stack[(unsigned char)S]);
    n = (signed long long int)((signed long)stack[(unsigned char)S - 1]);
    n += m << 32;
    vmPop;
    top = (signed long)(n / d);
    stack[(unsigned char)S] = (signed long)(n % d);
}
void slmod(void)
{
    if (top != 0) {
        WP = stack[(unsigned char)S] / top;
        stack[(unsigned char)S] %= top;
        top = WP;
    }
}
void mod(void)
{
    top = (top) ? stack[(unsigned char)S--] % top : stack[(unsigned char)S--];
}
void slash(void)
{
    top = (top) ? stack[(unsigned char)S--] / top : (stack[(unsigned char)S--], 0);
}
void umsta(void)
{
    d = (unsigned long long int)top;
    m = (unsigned long long int)stack[(unsigned char)S];
    m *= d;
    top = (unsigned long)(m >> 32);
    stack[(unsigned char)S] = (unsigned long)m;
}
void star(void)
{
    top *= stack[(unsigned char)S--];
}
void mstar(void)
{
    d = (signed long long int)top;
    m = (signed long long int)stack[(unsigned char)S];
    m *= d;
    top = (signed long)(m >> 32);
    stack[(unsigned char)S] = (signed long)m;
}
void ssmod(void)
{
    d = (signed long long int)top;
    m = (signed long long int)stack[(unsigned char)S];
    n = (signed long long int)stack[(unsigned char)(S - 1)];
    n *= m;
    vmPop;
    top = (signed long)(n / d);
    stack[(unsigned char)S] = (signed long)(n % d);
}
void stasl(void)
{
    d = (signed long long int)top;
    m = (signed long long int)stack[(unsigned char)S];
    n = (signed long long int)stack[(unsigned char)(S - 1)];
    n *= m;
    vmPop;
    vmPop;
    top = (signed long)(n / d);
}

void pick(void)
{
    top = stack[(unsigned char)(S - top)];
}

void pstor(void)
{
    data[top >> 2] += stack[(unsigned char)S--], vmPop;
}

void dstor(void)
{
    data[(top >> 2) + 1] = stack[(unsigned char)S--];
    data[top >> 2] = stack[(unsigned char)S--];
    vmPop;
}

void dat(void)
{
    vmPush data[top >> 2];
    top = data[(top >> 2) + 1];
}

void count(void)
{
    stack[(unsigned char)++S] = top + 1;
    top = cData[top];
}

void dovar(void)
{
    vmPush WP;
}

void maxx(void)
{
    if (top < stack[(unsigned char)S])
        vmPop;
    else
        (unsigned char)S--;
}

void minn(void)
{
    if (top < stack[(unsigned char)S])
        (unsigned char)S--;
    else
        vmPop;
}

void sendPacket(void)
{
}

void poke(void)
{
    Pointer = (long*)top;
    *Pointer = stack[(unsigned char)S--];
    vmPop;
}

void peeek(void)
{
    Pointer = (long*)top;
    top = *Pointer;
}

void adc(void)
{
    top = (long)analogRead(top);
}

void gfxinit(void)
{
    gfx_init();
}

void screenw(void)
{
    vmPush gfx_width();
}

void screenh(void)
{
    vmPush gfx_height();
}

void cls(void)
{
    gfx_clear(static_cast<uint16_t>(top));
    vmPop;
}

void pixel(void)
{
    long color = top;
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    gfx_pixel(static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<uint16_t>(color));
    vmPop;
}

void gfxline(void)
{
    long color = top;
    long y2 = stack[(unsigned char)S--];
    long x2 = stack[(unsigned char)S--];
    long y1 = stack[(unsigned char)S--];
    long x1 = stack[(unsigned char)S--];
    gfx_line(static_cast<int16_t>(x1), static_cast<int16_t>(y1), static_cast<int16_t>(x2), static_cast<int16_t>(y2), static_cast<uint16_t>(color));
    vmPop;
}

void rect(void)
{
    long color = top;
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    gfx_rect(static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(w), static_cast<int16_t>(h), static_cast<uint16_t>(color));
    vmPop;
}

void fillrect(void)
{
    long color = top;
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    gfx_fill_rect(static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(w), static_cast<int16_t>(h), static_cast<uint16_t>(color));
    vmPop;
}

void text(void)
{
    long color = top;
    long lenValue = stack[(unsigned char)S--];
    long addr = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    gfx_text(static_cast<int16_t>(x),
             static_cast<int16_t>(y),
             reinterpret_cast<const char*>(cData + addr),
             static_cast<size_t>(lenValue),
             static_cast<uint16_t>(color));
    vmPop;
}

void textw(void)
{
    long lenValue = top;
    long addr = stack[(unsigned char)S];
    top = gfx_text_width(reinterpret_cast<const char*>(cData + addr), static_cast<size_t>(lenValue));
    (unsigned char)S--;
}

void rotation(void)
{
    gfx_set_rotation(static_cast<uint8_t>(top));
    vmPop;
}

void textscale(void)
{
    gfx_set_text_scale(static_cast<uint8_t>(top));
    vmPop;
}

void bitmap1(void)
{
    long addr = top;
    long color = stack[(unsigned char)S--];
    long scale = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    gfx_bitmap_1bit(static_cast<int16_t>(x),
                    static_cast<int16_t>(y),
                    static_cast<int16_t>(w),
                    static_cast<int16_t>(h),
                    static_cast<uint8_t>(scale),
                    static_cast<uint16_t>(color),
                    reinterpret_cast<const uint32_t*>(&data[addr >> 2]));
    vmPop;
}

void touchupdate(void)
{
    top = gui_touch_update() LOGICAL;
}

void touchtype(void)
{
    vmPush gui_current_event().type;
}

void touchx(void)
{
    vmPush gui_current_event().x;
}

void touchy(void)
{
    vmPush gui_current_event().y;
}

void touchdx(void)
{
    vmPush gui_current_event().dx;
}

void touchdy(void)
{
    vmPush gui_current_event().dy;
}

void touchtick(void)
{
    vmPush static_cast<long>(gui_current_event().tick);
}

void touchtarget(void)
{
    vmPush gui_current_event().target;
}

void swipedetect(void)
{
    vmPush gui_swipe_detect();
}

void stylenew(void)
{
    vmPush gui_style_new();
}

void styleset(void)
{
    long style = top;
    long font = stack[(unsigned char)S--];
    long padding = stack[(unsigned char)S--];
    long border = stack[(unsigned char)S--];
    long bg = stack[(unsigned char)S--];
    long fg = stack[(unsigned char)S--];
    gui_style_set(static_cast<int16_t>(style),
                  static_cast<uint16_t>(fg),
                  static_cast<uint16_t>(bg),
                  static_cast<uint16_t>(border),
                  static_cast<uint16_t>(padding),
                  static_cast<uint16_t>(font));
    vmPop;
}

void stylefieldget(void)
{
    long field = top;
    long style = stack[(unsigned char)S];
    top = gui_style_field_get(static_cast<int16_t>(style), static_cast<int16_t>(field));
    (unsigned char)S--;
}

void stylefieldset(void)
{
    long field = top;
    long style = stack[(unsigned char)S--];
    long value = stack[(unsigned char)S--];
    gui_style_field_set(static_cast<int16_t>(style), static_cast<int16_t>(field), value);
    vmPop;
}

void nodenew(void)
{
    vmPush gui_node_new();
}

void nodeadd(void)
{
    long child = top;
    long parent = stack[(unsigned char)S--];
    gui_node_add(static_cast<int16_t>(parent), static_cast<int16_t>(child));
    vmPop;
}

void noderemove(void)
{
    gui_node_remove(static_cast<int16_t>(top));
    vmPop;
}

void nodedirty(void)
{
    gui_node_dirty(static_cast<int16_t>(top));
    vmPop;
}

void nodedraw(void)
{
    gui_node_draw(static_cast<int16_t>(top));
    vmPop;
}

void nodedrawtree(void)
{
    gui_node_draw_tree(static_cast<int16_t>(top));
    vmPop;
}

void nodehit(void)
{
    long y = top;
    long x = stack[(unsigned char)S--];
    long node = stack[(unsigned char)S];
    top = gui_node_hit(static_cast<int16_t>(node), static_cast<int16_t>(x), static_cast<int16_t>(y));
    (unsigned char)S--;
}

void nodescreenxy(void)
{
    int16_t x = 0;
    int16_t y = 0;
    gui_node_screen_xy(static_cast<int16_t>(top), &x, &y);
    top = x;
    vmPush y;
}

void nodesetbounds(void)
{
    long node = top;
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    gui_node_set_bounds(static_cast<int16_t>(node),
                        static_cast<int16_t>(x),
                        static_cast<int16_t>(y),
                        static_cast<int16_t>(w),
                        static_cast<int16_t>(h));
    vmPop;
}

void nodevisible(void)
{
    top = gui_node_visible(static_cast<int16_t>(top)) LOGICAL;
}

void nodetouchable(void)
{
    top = gui_node_touchable(static_cast<int16_t>(top)) LOGICAL;
}

void nodefieldget(void)
{
    long field = top;
    long node = stack[(unsigned char)S];
    top = gui_node_field_get(static_cast<int16_t>(node), static_cast<int16_t>(field));
    (unsigned char)S--;
}

void nodefieldset(void)
{
    long field = top;
    long node = stack[(unsigned char)S--];
    long value = stack[(unsigned char)S--];
    gui_node_field_set(static_cast<int16_t>(node), static_cast<int16_t>(field), value);
    vmPop;
}

void eventdispatch(void)
{
    long eventType = top;
    long node = stack[(unsigned char)S--];
    gui_event_dispatch(static_cast<int16_t>(node), static_cast<uint8_t>(eventType));
    vmPop;
}

void eventbubble(void)
{
    long eventType = top;
    long node = stack[(unsigned char)S--];
    gui_event_bubble(static_cast<int16_t>(node), static_cast<uint8_t>(eventType));
    vmPop;
}

void appnew(void)
{
    vmPush gui_app_new();
}

void appaddscreen(void)
{
    long screen = top;
    long app = stack[(unsigned char)S--];
    gui_app_add_screen(static_cast<int16_t>(app), static_cast<int16_t>(screen));
    vmPop;
}

void appnext(void)
{
    gui_app_next(static_cast<int16_t>(top));
    vmPop;
}

void appprev(void)
{
    gui_app_prev(static_cast<int16_t>(top));
    vmPop;
}

void appshow(void)
{
    long index = top;
    long app = stack[(unsigned char)S--];
    gui_app_show(static_cast<int16_t>(app), static_cast<int16_t>(index));
    vmPop;
}

void appactive(void)
{
    top = gui_app_active(static_cast<int16_t>(top));
}

void appdraw(void)
{
    gui_app_draw(static_cast<int16_t>(top));
    vmPop;
}

void wpanel(void)
{
    long style = top;
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_panel(static_cast<int16_t>(parent),
                           static_cast<int16_t>(x),
                           static_cast<int16_t>(y),
                           static_cast<int16_t>(w),
                           static_cast<int16_t>(h),
                           static_cast<int16_t>(style));
    (unsigned char)S--;
}

void wlabel(void)
{
    long lenValue = top;
    long addr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_label(static_cast<int16_t>(parent),
                           static_cast<int16_t>(x),
                           static_cast<int16_t>(y),
                           static_cast<int16_t>(w),
                           static_cast<int16_t>(h),
                           static_cast<int16_t>(style),
                           addr,
                           lenValue);
    (unsigned char)S--;
}

void wbutton(void)
{
    long eventXt = top;
    long lenValue = stack[(unsigned char)S--];
    long addr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_button(static_cast<int16_t>(parent),
                            static_cast<int16_t>(x),
                            static_cast<int16_t>(y),
                            static_cast<int16_t>(w),
                            static_cast<int16_t>(h),
                            static_cast<int16_t>(style),
                            addr,
                            lenValue,
                            eventXt);
    (unsigned char)S--;
}

void wstatus(void)
{
    long lenValue = top;
    long addr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_status(static_cast<int16_t>(parent),
                            static_cast<int16_t>(x),
                            static_cast<int16_t>(y),
                            static_cast<int16_t>(w),
                            static_cast<int16_t>(h),
                            static_cast<int16_t>(style),
                            addr,
                            lenValue);
    (unsigned char)S--;
}

void wvalue(void)
{
    long valueAddr = top;
    long lenValue = stack[(unsigned char)S--];
    long addr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_value(static_cast<int16_t>(parent),
                           static_cast<int16_t>(x),
                           static_cast<int16_t>(y),
                           static_cast<int16_t>(w),
                           static_cast<int16_t>(h),
                           static_cast<int16_t>(style),
                           addr,
                           lenValue,
                           valueAddr);
    (unsigned char)S--;
}

void wgauge(void)
{
    long maxValue = top;
    long minValue = stack[(unsigned char)S--];
    long valueAddr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_gauge(static_cast<int16_t>(parent),
                           static_cast<int16_t>(x),
                           static_cast<int16_t>(y),
                           static_cast<int16_t>(w),
                           static_cast<int16_t>(h),
                           static_cast<int16_t>(style),
                           valueAddr,
                           minValue,
                           maxValue);
    (unsigned char)S--;
}

void wchart(void)
{
    long maxValue = top;
    long minValue = stack[(unsigned char)S--];
    long count = stack[(unsigned char)S--];
    long pointsAddr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_chart(static_cast<int16_t>(parent),
                           static_cast<int16_t>(x),
                           static_cast<int16_t>(y),
                           static_cast<int16_t>(w),
                           static_cast<int16_t>(h),
                           static_cast<int16_t>(style),
                           pointsAddr,
                           count,
                           minValue,
                           maxValue);
    (unsigned char)S--;
}

void walarm(void)
{
    long valueAddr = top;
    long lenValue = stack[(unsigned char)S--];
    long addr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long h = stack[(unsigned char)S--];
    long w = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_alarm(static_cast<int16_t>(parent),
                           static_cast<int16_t>(x),
                           static_cast<int16_t>(y),
                           static_cast<int16_t>(w),
                           static_cast<int16_t>(h),
                           static_cast<int16_t>(style),
                           addr,
                           lenValue,
                           valueAddr);
    (unsigned char)S--;
}

void wicon16(void)
{
    long scale = top;
    long bitmapAddr = stack[(unsigned char)S--];
    long style = stack[(unsigned char)S--];
    long y = stack[(unsigned char)S--];
    long x = stack[(unsigned char)S--];
    long parent = stack[(unsigned char)S];
    top = gui_widget_icon16(static_cast<int16_t>(parent),
                            static_cast<int16_t>(x),
                            static_cast<int16_t>(y),
                            static_cast<int16_t>(style),
                            bitmapAddr,
                            scale);
    (unsigned char)S--;
}

void wifiStatusWord(void)
{
    replPrintLine(String("WiFi status: ") + networkStatusLine());
    replPrintLine(String("Saved credentials: ") + (networkHasSavedCredentials() ? "yes" : "no"));
    replPrintLine(String("Station connected: ") + (networkIsStationConnected() ? "yes" : "no"));
    replPrintLine(String("Setup AP active: ") + (networkIsApActive() ? "yes" : "no"));
    if (networkIsStationConnected()) {
        replPrintLine(String("Connected SSID: ") + networkConnectedSsid());
        replPrintLine(String("IP address: ") + networkStationIp());
    }
    if (networkIsApActive()) {
        replPrintLine(String("Setup AP: ") + networkApSsid());
        replPrintLine(String("AP password: ") + networkApPassword());
    }
    wifiPrintUrls();
}

void wifiScanWord(void)
{
    networkRequestScan();
    networkTick();
    replPrintLine(String("Visible networks: ") + String(networkScanCount()));
    for (int i = 0; i < networkScanCount(); ++i) {
        String line = String(i + 1) + ". " + networkScanSsid(i);
        line += " ";
        line += String(networkScanRssi(i));
        line += "dBm ";
        line += networkScanSecure(i) ? "secure" : "open";
        replPrintLine(line);
    }
}

void wifiConnectWord(void)
{
    long passLen = top;
    long passAddr = stack[(unsigned char)S--];
    long ssidLen = stack[(unsigned char)S--];
    long ssidAddr = stack[(unsigned char)S--];
    bool ok = networkConnect(forthString(ssidAddr, ssidLen), forthString(passAddr, passLen), true);
    top = ok ? -1 : 0;
}

void wifiWpsWord(void)
{
    top = networkStartWps() ? -1 : 0;
}

void wifiForgetWord(void)
{
    top = networkForgetCredentials() ? -1 : 0;
}

void wifiConnectedWord(void)
{
    top = networkIsStationConnected() ? -1 : 0;
}

void wifiApWord(void)
{
    top = networkIsApActive() ? -1 : 0;
}

void wifiSavedWord(void)
{
    top = networkHasSavedCredentials() ? -1 : 0;
}

void millisWord(void)
{
    vmPush static_cast<long>(data_millis_now());
}

void timeSyncWord(void)
{
    top = data_time_synced() ? -1 : 0;
}

void timeUnixWord(void)
{
    vmPush data_time_unix();
}

void timeHmsWord(void)
{
    int hour = 0;
    int minute = 0;
    int second = 0;
    data_time_hms(&hour, &minute, &second);
    vmPush hour;
    vmPush minute;
    vmPush second;
}

void timeYmdWord(void)
{
    int year = 0;
    int month = 0;
    int day = 0;
    data_time_ymd(&year, &month, &day);
    vmPush year;
    vmPush month;
    vmPush day;
}

void timeWdayWord(void)
{
    vmPush data_time_wday();
}

void timeMonthDaysWord(void)
{
    long month = top;
    long year = stack[(unsigned char)S];
    top = data_time_month_days(static_cast<int>(year), static_cast<int>(month));
    (unsigned char)S--;
}

void timeMonthFirstWord(void)
{
    long month = top;
    long year = stack[(unsigned char)S];
    top = data_time_month_first_wday(static_cast<int>(year), static_cast<int>(month));
    (unsigned char)S--;
}

void taskNewWord(void)
{
    vmPush data_task_new();
}

void taskEveryWord(void)
{
    long xt = top;
    long interval = stack[(unsigned char)S--];
    long task = stack[(unsigned char)S];
    data_task_every(static_cast<int16_t>(task), static_cast<uint32_t>(interval), xt);
    (unsigned char)S--;
}

void taskOnceWord(void)
{
    long xt = top;
    long interval = stack[(unsigned char)S--];
    long task = stack[(unsigned char)S];
    data_task_once(static_cast<int16_t>(task), static_cast<uint32_t>(interval), xt);
    (unsigned char)S--;
}

void taskStartWord(void)
{
    data_task_start(static_cast<int16_t>(top));
    vmPop;
}

void taskStopWord(void)
{
    data_task_stop(static_cast<int16_t>(top));
    vmPop;
}

void taskActiveWord(void)
{
    top = data_task_active(static_cast<int16_t>(top)) ? -1 : 0;
}

void localsEnterWord(void)
{
    const uint8_t localCount = static_cast<uint8_t>(data[IP >> 2]);
    IP += 4;
    const uint8_t inputCount = static_cast<uint8_t>(data[IP >> 2]);
    IP += 4;
    LocalFrame& frame = forthLocalFrames[forthCallDepth];
    frame.active = localCount > 0;
    frame.count = localCount;
    frame.returnSlot = static_cast<uint8_t>(R);
    for (uint8_t i = 0; i < localCount; ++i) {
        rack[(unsigned char)(frame.returnSlot + 1 + i)] = 0;
    }
    for (int i = static_cast<int>(inputCount) - 1; i >= 0; --i) {
        rack[(unsigned char)(frame.returnSlot + 1 + i)] = top;
        vmPop;
    }
    R = static_cast<unsigned char>(frame.returnSlot + localCount);
}

void localFetchWord(void)
{
    const long index = data[IP >> 2];
    IP += 4;
    const LocalFrame& frame = forthLocalFrames[forthCallDepth];
    vmPush rack[(unsigned char)(frame.returnSlot + 1 + index)];
}

void localSetWord(void)
{
    const long index = data[IP >> 2];
    IP += 4;
    LocalFrame& frame = forthLocalFrames[forthCallDepth];
    rack[(unsigned char)(frame.returnSlot + 1 + index)] = top;
    vmPop;
}

void localsEndWord(void)
{
    endLocalCompileScope();
}

void localsOpenWord(void)
{
    endLocalCompileScope();
    forthLocalsCompileActive = true;
    forthLocalsCompileCount = 0;
    forthLocalsCompileInputCount = 0;
    forthLocalsSavedCp = forthVar(FORTH_ADDR_CP);
    forthLocalsSavedLast = forthVar(FORTH_ADDR_LAST);
    forthLocalsSavedContext = forthVar(FORTH_ADDR_CONTEXT);

    String token;
    enum class LocalParseMode : uint8_t { Inputs, Scratch, Outputs };
    LocalParseMode mode = LocalParseMode::Inputs;
    while (parseNextInputToken(token)) {
        if (token == "}") {
            break;
        }
        if (token == "|") {
            if (mode == LocalParseMode::Inputs) {
                mode = LocalParseMode::Scratch;
            }
            continue;
        }
        if (token == "--") {
            mode = LocalParseMode::Outputs;
            continue;
        }
        if (mode == LocalParseMode::Outputs) {
            continue;
        }
        if (forthLocalsCompileCount >= MAX_LOCALS) {
            break;
        }
        token.toCharArray(localsCompileNames[forthLocalsCompileCount], MAX_LOCAL_NAME + 1);
        if (mode == LocalParseMode::Inputs) {
            ++forthLocalsCompileInputCount;
        }
        ++forthLocalsCompileCount;
    }

    if (forthLocalsCompileCount > 0) {
        compileCell(forthWordLocalsEnter);
        compileCell(forthLocalsCompileCount);
        compileCell(forthLocalsCompileInputCount);
    }
}

void compileWithLocalsWord(void)
{
    if (tokenEquals(top, "{")) {
        vmPop;
        localsOpenWord();
        return;
    }
    if (forthLocalsCompileActive) {
        if (tokenEquals(top, "TO")) {
            vmPop;
            String token;
            if (parseNextInputToken(token)) {
                for (uint8_t i = 0; i < forthLocalsCompileCount; ++i) {
                    if (token.equalsIgnoreCase(localsCompileNames[i])) {
                        compileCell(forthWordLocalSet);
                        compileCell(i);
                        return;
                    }
                }
            }
            return;
        }
        for (uint8_t i = 0; i < forthLocalsCompileCount; ++i) {
            if (tokenMatchesLocal(top, i)) {
                compileCell(forthWordLocalFetch);
                compileCell(i);
                vmPop;
                return;
            }
        }
    }
    P = forthWordScomp;
    WP = P + 4;
}

void (*primitives[157])(void) = {
    /* case 0 */ nop,
    /* case 1 */ accep,
    /* case 2 */ qrx,
    /* case 3 */ txsto,
    /* case 4 */ docon,
    /* case 5 */ dolit,
    /* case 6 */ dolist,
    /* case 7 */ exitt,
    /* case 8 */ execu,
    /* case 9 */ donext,
    /* case 10 */ qbran,
    /* case 11 */ bran,
    /* case 12 */ store,
    /* case 13 */ at,
    /* case 14 */ cstor,
    /* case 15 */ cat,
    /* case 16 */ nop,
    /* case 17 */ nop,
    /* case 18 */ rfrom,
    /* case 19 */ rat,
    /* case 20 */ tor,
    /* case 21 */ nop,
    /* case 22 */ nop,
    /* case 23 */ drop,
    /* case 24 */ dup,
    /* case 25 */ swap,
    /* case 26 */ over,
    /* case 27 */ zless,
    /* case 28 */ andd,
    /* case 29 */ orr,
    /* case 30 */ xorr,
    /* case 31 */ uplus,
    /* case 32 */ next,
    /* case 33 */ qdup,
    /* case 34 */ rot,
    /* case 35 */ ddrop,
    /* case 36 */ ddup,
    /* case 37 */ plus,
    /* case 38 */ inver,
    /* case 39 */ negat,
    /* case 40 */ dnega,
    /* case 41 */ subb,
    /* case 42 */ abss,
    /* case 43 */ equal,
    /* case 44 */ uless,
    /* case 45 */ less,
    /* case 46 */ ummod,
    /* case 47 */ msmod,
    /* case 48 */ slmod,
    /* case 49 */ mod,
    /* case 50 */ slash,
    /* case 51 */ umsta,
    /* case 52 */ star,
    /* case 53 */ mstar,
    /* case 54 */ ssmod,
    /* case 55 */ stasl,
    /* case 56 */ pick,
    /* case 57 */ pstor,
    /* case 58 */ dstor,
    /* case 59 */ dat,
    /* case 60 */ count,
    /* case 61 */ dovar,
    /* case 62 */ maxx,
    /* case 63 */ minn,
    /* case 64 */ nop,
    /* case 65 */ sendPacket,
    /* case 66 */ poke,
    /* case 67 */ peeek,
    /* case 68 */ adc,
    /* case 69 */ nop,
    /* case 70 */ nop,
    /* case 71 */ nop,
    /* case 72 */ gfxinit,
    /* case 73 */ screenw,
    /* case 74 */ screenh,
    /* case 75 */ cls,
    /* case 76 */ pixel,
    /* case 77 */ gfxline,
    /* case 78 */ rect,
    /* case 79 */ fillrect,
    /* case 80 */ text,
    /* case 81 */ textw,
    /* case 82 */ rotation,
    /* case 83 */ bitmap1,
    /* case 84 */ touchupdate,
    /* case 85 */ touchtype,
    /* case 86 */ touchx,
    /* case 87 */ touchy,
    /* case 88 */ touchdx,
    /* case 89 */ touchdy,
    /* case 90 */ touchtick,
    /* case 91 */ touchtarget,
    /* case 92 */ swipedetect,
    /* case 93 */ stylenew,
    /* case 94 */ styleset,
    /* case 95 */ stylefieldget,
    /* case 96 */ stylefieldset,
    /* case 97 */ nodenew,
    /* case 98 */ nodeadd,
    /* case 99 */ noderemove,
    /* case 100 */ nodedirty,
    /* case 101 */ nodedraw,
    /* case 102 */ nodedrawtree,
    /* case 103 */ nodehit,
    /* case 104 */ nodescreenxy,
    /* case 105 */ nodesetbounds,
    /* case 106 */ nodevisible,
    /* case 107 */ nodetouchable,
    /* case 108 */ nodefieldget,
    /* case 109 */ nodefieldset,
    /* case 110 */ eventdispatch,
    /* case 111 */ eventbubble,
    /* case 112 */ appnew,
    /* case 113 */ appaddscreen,
    /* case 114 */ appnext,
    /* case 115 */ appprev,
    /* case 116 */ appshow,
    /* case 117 */ appactive,
    /* case 118 */ appdraw,
    /* case 119 */ wpanel,
    /* case 120 */ wlabel,
    /* case 121 */ wbutton,
    /* case 122 */ wstatus,
    /* case 123 */ wvalue,
    /* case 124 */ wgauge,
    /* case 125 */ wchart,
    /* case 126 */ walarm,
    /* case 127 */ wicon16,
    /* case 128 */ wifiStatusWord,
    /* case 129 */ wifiScanWord,
    /* case 130 */ wifiConnectWord,
    /* case 131 */ wifiWpsWord,
    /* case 132 */ wifiForgetWord,
    /* case 133 */ wifiConnectedWord,
    /* case 134 */ wifiApWord,
    /* case 135 */ wifiSavedWord,
    /* case 136 */ millisWord,
    /* case 137 */ timeSyncWord,
    /* case 138 */ timeUnixWord,
    /* case 139 */ timeHmsWord,
    /* case 140 */ timeYmdWord,
    /* case 141 */ timeWdayWord,
    /* case 142 */ taskNewWord,
    /* case 143 */ taskEveryWord,
    /* case 144 */ taskOnceWord,
    /* case 145 */ taskStartWord,
    /* case 146 */ taskStopWord,
    /* case 147 */ taskActiveWord,
    /* case 148 */ timeMonthDaysWord,
    /* case 149 */ timeMonthFirstWord,
    /* case 150 */ textscale,
    /* case 151 */ localsEnterWord,
    /* case 152 */ localFetchWord,
    /* case 153 */ localsEndWord,
    /* case 154 */ localsOpenWord,
    /* case 155 */ compileWithLocalsWord,
    /* case 156 */ localSetWord
};
