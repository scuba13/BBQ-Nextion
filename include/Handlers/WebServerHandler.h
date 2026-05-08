#ifndef WEB_SERVER_CONTROL_H
#define WEB_SERVER_CONTROL_H

#include "SystemStatus.h"
#include "Handlers/FileSystem.h"
#include "Handlers/LogHandler.h"
#include "Handlers/OTAHandler.h"
#include "Handlers/DiagnosticsHandler.h"
#include "Handlers/MQTTHandler.h"

class WebServerControl {
public:
    WebServerControl(SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, AsyncWebServer& server, DiagnosticsHandler& diagnosticsHandler, MQTTHandler& mqttHandler);
    void begin();

private:
    SystemStatus& _systemStatus;
    FileSystem& _fileSystem;
    LogHandler& _logger;
    AsyncWebServer& _server;
    OTAHandler _otaHandler;
    DiagnosticsHandler& _diagnostics;
    MQTTHandler& _mqttHandler;
};

#endif
