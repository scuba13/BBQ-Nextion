#ifndef TEMPERATURE_ENDPOINTS_H
#define TEMPERATURE_ENDPOINTS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"

void registerTemperatureEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif // TEMPERATURE_ENDPOINTS_H
