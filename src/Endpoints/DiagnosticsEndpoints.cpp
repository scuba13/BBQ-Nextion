#include "DiagnosticsEndpoints.h"
#include "ResponseHelper.h"

void registerDiagnosticsEndpoints(AsyncWebServer& server, DiagnosticsHandler& diagnostics, LogHandler& logger) {
    server.on("/api/v1/diagnostics", HTTP_GET, [&](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando diagnósticos");

        auto metrics  = diagnostics.getMetrics();
        auto counters = diagnostics.getCounters();

        JsonDocument doc;
        doc["heap"]["free"]          = metrics.freeHeap;
        doc["heap"]["min"]           = metrics.minFreeHeap;
        doc["heap"]["maxAlloc"]      = metrics.maxAllocHeap;
        doc["heap"]["fragmentation"] = metrics.heapFragmentation;
        doc["cpu"]["freq"]           = metrics.cpuFreqMHz;
        doc["stack"]["free"]         = metrics.freeStack;
        doc["system"]["uptime"]      = metrics.uptime;
        doc["system"]["healthy"]     = diagnostics.isHealthy();
        doc["counters"]["wifiReconnections"] = counters.wifiReconnections;
        doc["counters"]["mqttReconnections"] = counters.mqttReconnections;
        doc["counters"]["sensorBBQErrors"]   = counters.sensorBBQErrors;
        doc["counters"]["sensorPrtErrors"]   = counters.sensorPrtErrors;
        doc["counters"]["sensorIntErrors"]   = counters.sensorIntErrors;
        doc["counters"]["relayEmergencies"]  = counters.relayEmergencies;

        ResponseHelper::sendJsonResponse(request, 200, "Diagnósticos obtidos com sucesso", doc.as<JsonObject>());
    });
} 