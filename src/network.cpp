#include <Arduino.h>

#include <ESP.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wps.h>

#include "network.h"

namespace {

constexpr char PREF_NAMESPACE[] = "fifthos-net";
constexpr char PREF_SSID[] = "ssid";
constexpr char PREF_PASS[] = "pass";
constexpr size_t WIFI_NAME_BUF_LEN = 33;
constexpr size_t WIFI_PASS_BUF_LEN = 65;
constexpr int MAX_SCAN_RESULTS = 8;
constexpr unsigned long CONNECT_TIMEOUT_MS = 12000;
constexpr uint16_t AP_CHANNEL = 1;

struct ScanResult {
    bool used;
    char ssid[WIFI_NAME_BUF_LEN];
    int32_t rssi;
    wifi_auth_mode_t auth;
};

Preferences prefs;
WiFiEventId_t wifiEventToken = 0;
esp_wps_config_t wpsConfig = WPS_CONFIG_INIT_DEFAULT(WPS_TYPE_PBC);

char savedSsid[WIFI_NAME_BUF_LEN] = {};
char savedPass[WIFI_PASS_BUF_LEN] = {};
char connectedSsid[WIFI_NAME_BUF_LEN] = {};
char apSsid[WIFI_NAME_BUF_LEN] = {};
char apPassword[16] = {};
const char* deviceHostName = "fifthos";

bool hasSavedCredentials = false;
bool staConnected = false;
bool apActive = false;
bool wpsActive = false;
bool wpsSavePending = false;
bool pendingSave = false;
bool pendingScan = true;
bool displayDirty = true;
bool mdnsStarted = false;

unsigned long connectStartedAt = 0;

char pendingSsid[WIFI_NAME_BUF_LEN] = {};
char pendingPass[WIFI_PASS_BUF_LEN] = {};
ScanResult scanResults[MAX_SCAN_RESULTS] = {};
int scanResultCount = 0;
String statusLine = "WiFi not configured";

void markDisplayDirty()
{
    displayDirty = true;
}

void updateStatus(const String& text)
{
    statusLine = text;
    Serial.println("[WiFi] " + text);
    markDisplayDirty();
}

void loadSavedCredentials()
{
    prefs.begin(PREF_NAMESPACE, true);
    String ssid = prefs.getString(PREF_SSID, "");
    String pass = prefs.getString(PREF_PASS, "");
    prefs.end();

    savedSsid[0] = 0;
    savedPass[0] = 0;
    ssid.toCharArray(savedSsid, sizeof(savedSsid));
    pass.toCharArray(savedPass, sizeof(savedPass));
    hasSavedCredentials = savedSsid[0] != 0;
}

void persistCredentials(const char* ssid, const char* pass)
{
    prefs.begin(PREF_NAMESPACE, false);
    prefs.putString(PREF_SSID, ssid);
    prefs.putString(PREF_PASS, pass);
    prefs.end();

    strlcpy(savedSsid, ssid, sizeof(savedSsid));
    strlcpy(savedPass, pass, sizeof(savedPass));
    hasSavedCredentials = savedSsid[0] != 0;
}

void clearSavedCredentialsInternal()
{
    prefs.begin(PREF_NAMESPACE, false);
    prefs.remove(PREF_SSID);
    prefs.remove(PREF_PASS);
    prefs.end();

    savedSsid[0] = 0;
    savedPass[0] = 0;
    hasSavedCredentials = false;
    wpsSavePending = false;
}

void generateSetupCredentials()
{
    uint64_t chipId = ESP.getEfuseMac();
    uint8_t hi = static_cast<uint8_t>(chipId >> 8);
    uint8_t lo = static_cast<uint8_t>(chipId);
    snprintf(apSsid, sizeof(apSsid), "FifthOS-%02X%02X", hi, lo);
    snprintf(apPassword, sizeof(apPassword), "fifthos%02X%02X", hi, lo);
}

void startMdnsIfNeeded()
{
    if (staConnected && !mdnsStarted) {
        if (MDNS.begin(deviceHostName)) {
            mdnsStarted = true;
        } else {
            Serial.println("[WiFi] mDNS start failed");
        }
    }
}

void stopMdnsIfNeeded()
{
    if (mdnsStarted) {
        MDNS.end();
        mdnsStarted = false;
    }
}

void startSetupAp()
{
    if (apActive) {
        return;
    }

    WiFi.mode(WIFI_AP_STA);
    if (WiFi.softAP(apSsid, apPassword, AP_CHANNEL, 0)) {
        apActive = true;
        updateStatus(String("Setup AP active: ") + apSsid);
    } else {
        updateStatus("Failed to start setup AP");
    }
}

void stopSetupAp()
{
    if (!apActive) {
        return;
    }
    WiFi.softAPdisconnect(true);
    apActive = false;
    markDisplayDirty();
}

void beginStationConnect(const char* ssid, const char* password, bool saveOnSuccess)
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false, false);
    delay(50);
    WiFi.begin(ssid, password);
    connectStartedAt = millis();
    pendingSave = saveOnSuccess;
    strlcpy(pendingSsid, ssid, sizeof(pendingSsid));
    strlcpy(pendingPass, password, sizeof(pendingPass));
    updateStatus(String("Connecting to ") + ssid);
}

void captureCurrentStaConfig()
{
    wifi_config_t config = {};
    if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK) {
        return;
    }

    const char* ssid = reinterpret_cast<const char*>(config.sta.ssid);
    const char* pass = reinterpret_cast<const char*>(config.sta.password);
    if (ssid[0] != 0) {
        persistCredentials(ssid, pass);
    }
}

void stopWps()
{
    if (!wpsActive) {
        return;
    }
    esp_wifi_wps_disable();
    wpsActive = false;
}

void performScan()
{
    pendingScan = false;
    scanResultCount = 0;
    memset(scanResults, 0, sizeof(scanResults));

    int found = WiFi.scanNetworks(false, true, false, 300, 0);
    if (found < 0) {
        updateStatus("WiFi scan failed");
        return;
    }

    for (int i = 0; i < found && scanResultCount < MAX_SCAN_RESULTS; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) {
            continue;
        }
        scanResults[scanResultCount].used = true;
        ssid.toCharArray(scanResults[scanResultCount].ssid, sizeof(scanResults[scanResultCount].ssid));
        scanResults[scanResultCount].rssi = WiFi.RSSI(i);
        scanResults[scanResultCount].auth = WiFi.encryptionType(i);
        scanResultCount++;
    }

    WiFi.scanDelete();
    markDisplayDirty();
}

void handleWiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            staConnected = true;
            strlcpy(connectedSsid, WiFi.SSID().c_str(), sizeof(connectedSsid));
            if (pendingSave || wpsSavePending) {
                if (wpsSavePending) {
                    captureCurrentStaConfig();
                } else if (pendingSsid[0] != 0) {
                    persistCredentials(pendingSsid, pendingPass);
                }
            }
            pendingSave = false;
            wpsSavePending = false;
            pendingSsid[0] = 0;
            pendingPass[0] = 0;
            stopWps();
            startMdnsIfNeeded();
            stopSetupAp();
            updateStatus(String("Connected to ") + connectedSsid);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            staConnected = false;
            connectedSsid[0] = 0;
            stopMdnsIfNeeded();
            if (wpsActive) {
                updateStatus("WPS in progress");
            } else {
                updateStatus(hasSavedCredentials ? "WiFi disconnected" : "WiFi not configured");
            }
            break;

        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            updateStatus("WPS succeeded, finalising WiFi");
            wpsSavePending = true;
            stopWps();
            delay(10);
            WiFi.begin();
            break;

        case ARDUINO_EVENT_WPS_ER_FAILED:
            wpsSavePending = false;
            stopWps();
            updateStatus("WPS failed");
            startSetupAp();
            break;

        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            wpsSavePending = false;
            stopWps();
            updateStatus("WPS timeout");
            startSetupAp();
            break;

        case ARDUINO_EVENT_WPS_ER_PIN:
            updateStatus("WPS pin offered");
            break;

        case ARDUINO_EVENT_WIFI_AP_START:
            apActive = true;
            markDisplayDirty();
            break;

        case ARDUINO_EVENT_WIFI_AP_STOP:
            apActive = false;
            markDisplayDirty();
            break;

        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            markDisplayDirty();
            break;

        default:
            break;
    }
}

} // namespace

void setupNetwork(int monitorSpeed, const char* deviceName)
{
    deviceHostName = deviceName;
    Serial.begin(monitorSpeed);
    delay(100);

    generateSetupCredentials();
    loadSavedCredentials();

    wifiEventToken = WiFi.onEvent(handleWiFiEvent);
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);

    if (hasSavedCredentials) {
        beginStationConnect(savedSsid, savedPass, false);
    } else {
        startSetupAp();
        updateStatus("No saved WiFi, setup AP ready");
    }
}

void networkTick()
{
    if (!staConnected && hasSavedCredentials && connectStartedAt != 0 && millis() - connectStartedAt > CONNECT_TIMEOUT_MS && !apActive && !wpsActive) {
        startSetupAp();
        connectStartedAt = 0;
        updateStatus(String("Could not join ") + savedSsid + ", setup AP ready");
    }

    if (!staConnected && !apActive && !wpsActive) {
        startSetupAp();
    }

    if (pendingScan) {
        performScan();
    }
}

bool networkIsStationConnected()
{
    return staConnected && WiFi.status() == WL_CONNECTED;
}

bool networkIsApActive()
{
    return apActive;
}

bool networkHasSavedCredentials()
{
    return hasSavedCredentials;
}

bool networkShouldShowSetupScreen()
{
    return !networkIsStationConnected();
}

bool networkTakeDisplayDirty()
{
    bool dirty = displayDirty;
    displayDirty = false;
    return dirty;
}

const char* networkConnectedSsid()
{
    return connectedSsid;
}

String networkStationIp()
{
    return networkIsStationConnected() ? WiFi.localIP().toString() : String("");
}

const char* networkApSsid()
{
    return apSsid;
}

const char* networkApPassword()
{
    return apPassword;
}

String networkApUrl()
{
    return String("http://") + WiFi.softAPIP().toString() + "/";
}

String networkReplUrl()
{
    if (networkIsStationConnected()) {
        return String("http://") + deviceHostName + ".local/";
    }
    return networkApUrl();
}

String networkSetupUrl()
{
    if (networkIsStationConnected()) {
        return String("http://") + deviceHostName + ".local/wifi";
    }
    return String("http://") + WiFi.softAPIP().toString() + "/wifi";
}

String networkStatusLine()
{
    return statusLine;
}

void networkRequestScan()
{
    pendingScan = true;
}

int networkScanCount()
{
    return scanResultCount;
}

const char* networkScanSsid(int index)
{
    if (index < 0 || index >= scanResultCount) {
        return "";
    }
    return scanResults[index].ssid;
}

int32_t networkScanRssi(int index)
{
    if (index < 0 || index >= scanResultCount) {
        return 0;
    }
    return scanResults[index].rssi;
}

bool networkScanSecure(int index)
{
    if (index < 0 || index >= scanResultCount) {
        return false;
    }
    return scanResults[index].auth != WIFI_AUTH_OPEN;
}

bool networkConnect(const String& ssid, const String& password, bool save)
{
    if (!ssid.length()) {
        updateStatus("WiFi connect rejected: missing SSID");
        return false;
    }

    beginStationConnect(ssid.c_str(), password.c_str(), save);
    return true;
}

bool networkStartWps()
{
    if (wpsActive) {
        return true;
    }

    WiFi.mode(WIFI_MODE_STA);
    stopSetupAp();
    if (esp_wifi_wps_enable(&wpsConfig) != ESP_OK) {
        updateStatus("WPS enable failed");
        startSetupAp();
        return false;
    }
    if (esp_wifi_wps_start(0) != ESP_OK) {
        esp_wifi_wps_disable();
        updateStatus("WPS start failed");
        startSetupAp();
        return false;
    }

    wpsActive = true;
    updateStatus("WPS started, press router button");
    return true;
}

bool networkForgetCredentials()
{
    clearSavedCredentialsInternal();
    stopWps();
    WiFi.disconnect(false, true);
    connectedSsid[0] = 0;
    staConnected = false;
    startSetupAp();
    networkRequestScan();
    updateStatus("Saved WiFi forgotten");
    return true;
}
