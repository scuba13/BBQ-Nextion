#ifndef GENERAL_ENDPOINTS_H
#define GENERAL_ENDPOINTS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "OTAHandler.h"

void registerGeneralEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, OTAHandler& otaHandler);

#endif // GENERAL_ENDPOINTS_H
