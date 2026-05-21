#include <Arduino.h>

#include "SPIFFS.h"

#include "config.h"
#include "credentials.h"
#include "dictionary.h"
#include "forth_runtime.h"
#include "gfx_backend.h"
#include "globals.h"
#include "gui_boot.h"
#include "gui_runtime.h"
#include "http.h"
#include "network.h"
#include "util.h"
#include "vm.h"

namespace {

constexpr uint16_t COLOR_BLACK = 0x0000;
constexpr uint16_t COLOR_WHITE = 0xFFFF;
constexpr uint16_t COLOR_RED = 0xF800;
constexpr uint16_t COLOR_GREEN = 0x07E0;
constexpr uint16_t COLOR_ORANGE = 0xFD20;

}

void printNetworkInfo()
{
    Serial.println();
    Serial.println();
    Serial.print("Connected to: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Navigate to http://");
    Serial.print(deviceName);
    Serial.println(".local");
}

static void drawBootStatus(const char* title, const String& detail, uint16_t titleColor, uint16_t detailColor)
{
    gfx_clear(COLOR_BLACK);
    gfx_set_text_scale(2);
    gfx_text(12, 24, title, strlen(title), titleColor);

    gfx_set_text_scale(2);
    String body = detail;
    if (body.length() == 0) {
        body = "no detail";
    }

    int16_t y = 72;
    int start = 0;
    while (start < body.length() && y < gfx_height() - 24) {
        int end = body.indexOf('\n', start);
        if (end < 0) {
            end = body.length();
        }
        String line = body.substring(start, end);
        if (line.length() > 16) {
            line = line.substring(0, 16);
        }
        gfx_text(12, y, line.c_str(), line.length(), detailColor);
        y += 28;
        start = end + 1;
    }
}

static void returnFail(String msg)
{
    server.send(500, "text/plain", msg + "\r\n");
}

static void handleInput()
{
    if (!server.hasArg("cmd")) {
        return returnFail("Missing Input");
    }
    HTTPin = server.arg("cmd");
    HTTPout = "";
    Serial.println(HTTPin); // line cleaned up
    forth_eval_buffer(HTTPin.c_str(), HTTPin.length());
    server.setContentLength(HTTPout.length());
    server.send(200, "text/plain", HTTPout);
}

void setup()
{
    P = 0x180;
    WP = 0x184;
    IP = 0;
    S = 0;
    R = 0;
    top = 0;
    cData = (uint8_t*)data;

    setupNetwork(monitorSpeed, ssid, pass, deviceName);
    initDictionary();
    gui_runtime_init();
    bool bootOk = true;
    for (size_t i = 0; i < fifthos_gui_boot_phase_count; i++) {
        drawBootStatus("FifthOS boot", fifthos_gui_boot_phases[i].name, COLOR_ORANGE, COLOR_WHITE);
        HTTPout = "";
        forth_eval_text(fifthos_gui_boot_phases[i].script);
        if (HTTPout.indexOf('?') >= 0) {
            drawBootStatus("GUI boot failed", String(fifthos_gui_boot_phases[i].name) + "\n" + HTTPout, COLOR_RED, COLOR_WHITE);
            bootOk = false;
            break;
        }
    }

    if (bootOk && !gui_has_active_app()) {
        drawBootStatus("GUI boot failed", "no active app", COLOR_RED, COLOR_WHITE);
        bootOk = false;
    }

    if (bootOk) {
        String detail = gui_has_active_app() ? "app built\nrender skipped" : "no active app";
        Serial.println(detail);
        gui_draw_active_app();
    }

    setupHttp(handleInput);
    printNetworkInfo();
}

void loop()
{
    server.handleClient();
    gui_runtime_tick();
}
