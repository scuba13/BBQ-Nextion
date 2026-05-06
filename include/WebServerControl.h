#ifndef WebServerControl_h
#define WebServerControl_h

#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"
#include "OTAHandler.h"
#include "DiagnosticsHandler.h"
#include "MQTTHandler.h"

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
