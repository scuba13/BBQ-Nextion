#ifndef SYSTEM_ENDPOINTS_H
#define SYSTEM_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "FileSystem.h"

void registerSystemEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif // SYSTEM_ENDPOINTS_H
