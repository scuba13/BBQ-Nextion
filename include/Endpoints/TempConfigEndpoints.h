#ifndef TEMP_CONFIG_ENDPOINTS_H
#define TEMP_CONFIG_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/FileSystem.h"
#include "Handlers/LogHandler.h"

void registerTempConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger);

#endif // TEMP_CONFIG_ENDPOINTS_H
