#ifndef MONITOR_ENDPOINTS_H
#define MONITOR_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/LogHandler.h"

void registerMonitorEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif
