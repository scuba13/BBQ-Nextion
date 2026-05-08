#ifndef TEMPERATURE_ENDPOINTS_H
#define TEMPERATURE_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/LogHandler.h"

void registerTemperatureEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif // TEMPERATURE_ENDPOINTS_H
