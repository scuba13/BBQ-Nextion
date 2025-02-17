#ifndef DIAGNOSTICS_ENDPOINTS_H
#define DIAGNOSTICS_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "DiagnosticsHandler.h"
#include "LogHandler.h"

void registerDiagnosticsEndpoints(AsyncWebServer& server, DiagnosticsHandler& diagnostics, LogHandler& logger);

#endif 