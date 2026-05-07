#ifndef DIAGNOSTICS_ENDPOINTS_H
#define DIAGNOSTICS_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "DiagnosticsHandler.h"
#include "LogHandler.h"
#include "SystemStatus.h"
#include "MQTTHandler.h"

void registerDiagnosticsEndpoints(AsyncWebServer& server,
                                   DiagnosticsHandler& diagnostics,
                                   LogHandler& logger,
                                   SystemStatus& systemStatus,
                                   MQTTHandler& mqttHandler);

#endif