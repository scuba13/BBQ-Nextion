#ifndef WebServerControl_h
#define WebServerControl_h

#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"
#include <ESPAsyncWebServer.h>

class WebServerControl {
public:
    WebServerControl(SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, AsyncWebServer& server);
    void begin();
    void loop();

private:
    SystemStatus& _systemStatus;
    FileSystem& _fileSystem;
    LogHandler& _logger;
    AsyncWebServer& _server;
};

#endif
