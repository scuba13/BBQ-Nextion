#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"
#include "WebServerControl.h"
#include "MQTTHandler.h"
#include <ESPAsyncWebServer.h>

class AppContext {
public:
    SystemStatus sysStat;
    FileSystem fileSystem;
    LogHandler logHandler;
    AsyncWebServer server;
    WebServerControl webServerControl;
    MQTTHandler mqttHandler;

    AppContext() 
        : sysStat()
        , fileSystem()
        , logHandler()
        , server(80)
        , webServerControl(sysStat, fileSystem, logHandler, server)
        , mqttHandler(sysStat, logHandler, fileSystem)
    {}
};

extern AppContext app;  // Declaração externa

#endif 