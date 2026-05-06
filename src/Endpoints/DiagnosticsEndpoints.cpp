#include "DiagnosticsEndpoints.h"
#include "ResponseHelper.h"

void registerDiagnosticsEndpoints(AsyncWebServer& server, DiagnosticsHandler& diagnostics, LogHandler& logger) {
    server.on("/api/v1/diagnostics", HTTP_GET, [&](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Fetching diagnostics");
        
        auto metrics = diagnostics.getMetrics();
        
        JsonDocument doc;
        doc["heap"]["free"] = metrics.freeHeap;
        doc["heap"]["min"] = metrics.minFreeHeap;
        doc["heap"]["maxAlloc"] = metrics.maxAllocHeap;
        doc["heap"]["fragmentation"] = metrics.heapFragmentation;
        doc["cpu"]["freq"] = metrics.cpuFreqMHz;
        doc["stack"]["free"] = metrics.freeStack;
        doc["system"]["uptime"] = metrics.uptime;
        doc["system"]["healthy"] = diagnostics.isHealthy();
        
        ResponseHelper::sendJsonResponse(request, 200, "Diagnostics fetched successfully", doc.as<JsonObject>());
    });
} 