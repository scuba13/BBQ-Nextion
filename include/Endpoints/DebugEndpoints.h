#ifndef DEBUG_ENDPOINTS_H
#define DEBUG_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/LogHandler.h"

void registerDebugEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger);

#endif // DEBUG_ENDPOINTS_H
