#pragma once

#include <WebServer.h>
#include "ESP32HTTPUpdateServer.h"
#include <ArduinoJson.h>

#include "JsonConfiguration.h"

class HttpServer
{
public:
    HttpServer();
    virtual ~HttpServer();

    void setup(void);
    void handle(void);

    String getContentType(String filename);
    bool   handleFileRead(String path);

    WebServer& webServer();

    void sendJson(const uint16_t code, JsonDocument& doc);

protected:
    static void get_config();
    static void set_config();

private:
    WebServer             _webServer;
    ESP32HTTPUpdateServer _httpUpdater;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern HttpServer HTTPServer;
#endif
