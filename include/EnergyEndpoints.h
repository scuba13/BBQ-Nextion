#ifndef ENERGY_ENDPOINTS_H
#define ENERGY_ENDPOINTS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"

void registerEnergyEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif // ENERGY_ENDPOINTS_H
