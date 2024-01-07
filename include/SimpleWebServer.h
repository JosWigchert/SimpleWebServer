#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <map>

#include "elements/BaseElement.h"
#include "Position.h"
#include "ValueInfo.h"

class SimpleWebServer
{
public:
    SimpleWebServer();
    ~SimpleWebServer();
    void begin();
    void stop();
    void addHTMLElement(Position position, BaseElement *element);
    void removeHTMLElement(Position position);
    void addValueWatch(const char *id, ValueInfo valueInfo);
    void addValueWatch(const char *id, void *valuePtr, ValueType valueType);
    void removeValueWatch(const char *id);

    void pushUpdate();

private:
    const uint16_t CLIENT_COUNT = 10;

    AsyncWebServer server;
    AsyncEventSource events;

    String responseContent;

    std::map<Position, BaseElement *> elements;
    std::map<const char *, ValueInfo> values;

    void handleRoot(AsyncWebServerRequest *request);
    void handleValues(AsyncWebServerRequest *request);
    void handleCallback(AsyncWebServerRequest *request);
    void handleFile(AsyncWebServerRequest *request);
    void handleEvent(AsyncEventSourceClient *client);
    String generateHTML();
};
