#ifndef MONITOR_ENDPOINTS_H
#define MONITOR_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"

void registerMonitorEndpoints(AsyncWebServer& server, SystemStatus& systemStatus);

#endif // MONITOR_ENDPOINTS_H
