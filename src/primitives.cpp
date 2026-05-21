#include <Arduino.h>
#include "globals.h"
#include "gfx_backend.h"
#include "gui_runtime.h"
#include "vm.h"
#include "common.h"
#include "http.h"

/******************************************************************************/
/* PRIMITIVES                                                                 */
/******************************************************************************/

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
    rack[(unsigned char)++R] = IP;
    IP = WP;
    next();
}

void exitt(void)
{
    IP = (long)rack[(unsigned char)R--];
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
void (*primitives[126])(void) = {
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
    /* case 83 */ touchupdate,
    /* case 84 */ touchtype,
    /* case 85 */ touchx,
    /* case 86 */ touchy,
    /* case 87 */ touchdx,
    /* case 88 */ touchdy,
    /* case 89 */ touchtick,
    /* case 90 */ touchtarget,
    /* case 91 */ swipedetect,
    /* case 92 */ stylenew,
    /* case 93 */ styleset,
    /* case 94 */ stylefieldget,
    /* case 95 */ stylefieldset,
    /* case 96 */ nodenew,
    /* case 97 */ nodeadd,
    /* case 98 */ noderemove,
    /* case 99 */ nodedirty,
    /* case 100 */ nodedraw,
    /* case 101 */ nodedrawtree,
    /* case 102 */ nodehit,
    /* case 103 */ nodescreenxy,
    /* case 104 */ nodesetbounds,
    /* case 105 */ nodevisible,
    /* case 106 */ nodetouchable,
    /* case 107 */ nodefieldget,
    /* case 108 */ nodefieldset,
    /* case 109 */ eventdispatch,
    /* case 110 */ eventbubble,
    /* case 111 */ appnew,
    /* case 112 */ appaddscreen,
    /* case 113 */ appnext,
    /* case 114 */ appprev,
    /* case 115 */ appshow,
    /* case 116 */ appactive,
    /* case 117 */ appdraw,
    /* case 118 */ wpanel,
    /* case 119 */ wlabel,
    /* case 120 */ wbutton,
    /* case 121 */ wstatus,
    /* case 122 */ wvalue,
    /* case 123 */ wgauge,
    /* case 124 */ wchart,
    /* case 125 */ walarm
};
