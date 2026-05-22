#include <Arduino.h>
#include <WebServer.h>

#include "index_html.h"
#include "network.h"
#include "wifi_setup_html.h"

WebServer server(80);
String HTTPin;
String HTTPout;

void setupHttp(void (*handleInput)())
{
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });
    server.on("/wifi", HTTP_GET, []() {
        server.send(200, "text/html", wifi_setup_html);
    });
    server.on("/input", HTTP_POST, [handleInput]() {
        handleInput();
    });
    server.begin();
}
