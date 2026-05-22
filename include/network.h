#pragma once

#include <Arduino.h>

void setupNetwork(int monitorSpeed, const char* deviceName);
void networkTick();

bool networkIsStationConnected();
bool networkIsApActive();
bool networkHasSavedCredentials();
bool networkShouldShowSetupScreen();
bool networkTakeDisplayDirty();

const char* networkConnectedSsid();
String networkStationIp();
const char* networkApSsid();
const char* networkApPassword();
String networkApUrl();
String networkReplUrl();
String networkSetupUrl();
String networkStatusLine();

void networkRequestScan();
int networkScanCount();
const char* networkScanSsid(int index);
int32_t networkScanRssi(int index);
bool networkScanSecure(int index);

bool networkConnect(const String& ssid, const String& password, bool save);
bool networkStartWps();
bool networkForgetCredentials();
