#ifndef TEMP_CONFIG_ENDPOINTS_H
#define TEMP_CONFIG_ENDPOINTS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"

void registerTempConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger);

#endif // TEMP_CONFIG_ENDPOINTS_H
