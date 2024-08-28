#ifndef MonitorEndpoints_h
#define MonitorEndpoints_h

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"

void registerMonitorEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif
