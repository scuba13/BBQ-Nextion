#ifndef DIAGNOSTICS_ENDPOINTS_H
#define DIAGNOSTICS_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "Handlers/DiagnosticsHandler.h"
#include "Handlers/LogHandler.h"
#include "SystemStatus.h"
#include "Handlers/MQTTHandler.h"

void registerDiagnosticsEndpoints(AsyncWebServer& server,
                                   DiagnosticsHandler& diagnostics,
                                   LogHandler& logger,
                                   SystemStatus& systemStatus,
                                   MQTTHandler& mqttHandler);

#endif