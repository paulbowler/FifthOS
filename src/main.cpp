#include <Arduino.h>

#include "SPIFFS.h"

#include "config.h"
#include "data_runtime.h"
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
    if (networkIsStationConnected()) {
        Serial.print("Connected to: ");
        Serial.println(networkConnectedSsid());
        Serial.print("IP address: ");
        Serial.println(networkStationIp());
    } else {
        Serial.println("Running without external WiFi");
    }
    if (networkIsApActive()) {
        Serial.print("Setup AP: ");
        Serial.println(networkApSsid());
        Serial.print("AP password: ");
        Serial.println(networkApPassword());
    }
    Serial.print("REPL URL: ");
    Serial.println(networkReplUrl());
    Serial.print("WiFi setup URL: ");
    Serial.println(networkSetupUrl());
}

static void drawBootStatus(const char* title, const String& detail, uint16_t titleColor, uint16_t detailColor)
{
    auto drawWrappedLine = [&](const String& rawLine, int16_t& y) {
        const int16_t x = 12;
        const int16_t maxWidth = gfx_width() - (x * 2);
        int start = 0;

        while (start < rawLine.length() && y < gfx_height() - 24) {
            int bestEnd = start + 1;
            int lastSpace = -1;

            for (int end = start + 1; end <= rawLine.length(); ++end) {
                if (rawLine[end - 1] == ' ') {
                    lastSpace = end - 1;
                }

                String chunk = rawLine.substring(start, end);
                if (gfx_text_width(chunk.c_str(), chunk.length()) > maxWidth) {
                    if (lastSpace >= start) {
                        bestEnd = lastSpace;
                    } else {
                        bestEnd = end - 1;
                    }
                    break;
                }

                bestEnd = end;
            }

            if (bestEnd <= start) {
                bestEnd = start + 1;
            }

            String chunk = rawLine.substring(start, bestEnd);
            chunk.trim();
            gfx_text(x, y, chunk.c_str(), chunk.length(), detailColor);
            y += 28;
            start = bestEnd;
            while (start < rawLine.length() && rawLine[start] == ' ') {
                start++;
            }
        }
    };

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
        drawWrappedLine(body.substring(start, end), y);
        start = end + 1;
    }
}

static void returnFail(String msg)
{
    server.send(500, "text/plain", msg + "\r\n");
}

static String jsonEscape(const String& value)
{
    String out;
    out.reserve(value.length() + 8);
    for (size_t i = 0; i < value.length(); ++i) {
        char c = value[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static void sendJson(const String& body)
{
    server.send(200, "application/json", body);
}

static String buildNetworkStatusJson()
{
    String body = "{";
    body += "\"connected\":";
    body += networkIsStationConnected() ? "true" : "false";
    body += ",\"ap_active\":";
    body += networkIsApActive() ? "true" : "false";
    body += ",\"saved\":";
    body += networkHasSavedCredentials() ? "true" : "false";
    body += ",\"status\":\"" + jsonEscape(networkStatusLine()) + "\"";
    body += ",\"connected_ssid\":\"" + jsonEscape(String(networkConnectedSsid())) + "\"";
    body += ",\"station_ip\":\"" + jsonEscape(networkStationIp()) + "\"";
    body += ",\"ap_ssid\":\"" + jsonEscape(String(networkApSsid())) + "\"";
    body += ",\"ap_password\":\"" + jsonEscape(String(networkApPassword())) + "\"";
    body += ",\"repl_url\":\"" + jsonEscape(networkReplUrl()) + "\"";
    body += ",\"setup_url\":\"" + jsonEscape(networkSetupUrl()) + "\"";
    body += ",\"selected_ssid\":\"\"";
    body += ",\"scan\":[";
    for (int i = 0; i < networkScanCount(); ++i) {
        if (i) {
            body += ",";
        }
        body += "{";
        body += "\"ssid\":\"" + jsonEscape(String(networkScanSsid(i))) + "\"";
        body += ",\"rssi\":";
        body += String(networkScanRssi(i));
        body += ",\"secure\":";
        body += networkScanSecure(i) ? "true" : "false";
        body += "}";
    }
    body += "]}";
    return body;
}

static void handleWifiStatus()
{
    sendJson(buildNetworkStatusJson());
}

static void handleWifiScan()
{
    networkRequestScan();
    networkTick();
    sendJson(buildNetworkStatusJson());
}

static void handleWifiConnect()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    bool ok = networkConnect(ssid, password, true);
    sendJson(String("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":\"" +
             jsonEscape(ok ? String("Connecting to ") + ssid : "Missing SSID") + "\"}");
}

static void handleWifiWps()
{
    bool ok = networkStartWps();
    sendJson(String("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":\"" +
             jsonEscape(ok ? "WPS started. Press the router WPS button now." : "WPS start failed") + "\"}");
}

static void handleWifiForget()
{
    bool ok = networkForgetCredentials();
    sendJson(String("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":\"" +
             jsonEscape(ok ? "Saved WiFi forgotten." : "Could not forget WiFi") + "\"}");
}

static void drawNetworkSetupScreen()
{
    String detail = networkStatusLine();
    detail += "\nREPL " + networkReplUrl();
    detail += "\nSetup " + networkSetupUrl();
    if (networkIsApActive()) {
        detail += "\nAP " + String(networkApSsid());
        detail += "\nWiFi pass " + String(networkApPassword());
    }
    if (networkScanCount() > 0) {
        detail += "\nNetworks:";
        for (int i = 0; i < networkScanCount(); ++i) {
            detail += "\n";
            detail += String(i + 1) + ". ";
            detail += networkScanSsid(i);
            detail += " ";
            detail += String(networkScanRssi(i));
            detail += "dBm";
        }
    }
    drawBootStatus("WiFi setup", detail, COLOR_ORANGE, COLOR_WHITE);
}

static void refreshDisplayMode(bool force)
{
    static bool showingSetup = false;
    bool shouldShowSetup = networkShouldShowSetupScreen();
    bool networkDirty = networkTakeDisplayDirty();
    if (!force && showingSetup == shouldShowSetup && !networkDirty) {
        return;
    }
    if (shouldShowSetup) {
        drawNetworkSetupScreen();
    } else if (gui_has_active_app() && (force || showingSetup != shouldShowSetup)) {
        gui_draw_active_app();
    }
    showingSetup = shouldShowSetup;
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

    setupNetwork(monitorSpeed, deviceName);
    data_runtime_init();
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
    }

    setupHttp(handleInput);
    server.on("/wifi/status", HTTP_GET, handleWifiStatus);
    server.on("/wifi/scan", HTTP_POST, handleWifiScan);
    server.on("/wifi/connect", HTTP_POST, handleWifiConnect);
    server.on("/wifi/wps", HTTP_POST, handleWifiWps);
    server.on("/wifi/forget", HTTP_POST, handleWifiForget);
    refreshDisplayMode(true);
    printNetworkInfo();
}

void loop()
{
    networkTick();
    data_runtime_tick();
    server.handleClient();
    refreshDisplayMode(false);
    gui_runtime_tick();
}
