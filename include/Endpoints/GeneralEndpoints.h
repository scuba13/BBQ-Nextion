#ifndef GENERAL_ENDPOINTS_H
#define GENERAL_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/LogHandler.h"
#include "Handlers/OTAHandler.h"

void registerGeneralEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, OTAHandler& otaHandler);

#endif // GENERAL_ENDPOINTS_H
