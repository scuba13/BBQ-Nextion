#include "DiagnosticsEndpoints.h"
#include "ResponseHelper.h"
#include "TaskHandler.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_system.h>

static const char* resetReasonString() {
    static const char* reasons[] = {
        "desconhecido", "power-on", "reset externo", "software",
        "panic/exception", "watchdog int.", "watchdog task",
        "watchdog outros", "sleep profundo", "brownout", "SDIO"
    };
    int r = (int)esp_reset_reason();
    if (r < 0 || r > 10) r = 0;
    return reasons[r];
}

void registerDiagnosticsEndpoints(AsyncWebServer& server,
                                   DiagnosticsHandler& diagnostics,
                                   LogHandler& logger,
                                   SystemStatus& systemStatus,
                                   MQTTHandler& mqttHandler) {

    server.on("/api/v1/diagnostics", HTTP_GET, [&](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando diagnósticos");

        auto metrics  = diagnostics.getMetrics();
        auto counters = diagnostics.getCounters();
        auto stacks   = getTaskStackWatermarks();

        bool wifiOk = (WiFi.status() == WL_CONNECTED);

        JsonDocument doc;

        // Heap
        doc["heap"]["free"]          = metrics.freeHeap;
        doc["heap"]["min"]           = metrics.minFreeHeap;
        doc["heap"]["maxAlloc"]      = metrics.maxAllocHeap;
        doc["heap"]["fragmentation"] = metrics.heapFragmentation;

        // CPU
        doc["cpu"]["freq"] = metrics.cpuFreqMHz;

        // Sistema
        doc["system"]["uptime"]      = metrics.uptime;
        doc["system"]["healthy"]     = diagnostics.isHealthy();
        doc["system"]["resetReason"] = resetReasonString();

        // WiFi
        doc["wifi"]["connected"]      = wifiOk;
        doc["wifi"]["rssi"]           = wifiOk ? WiFi.RSSI() : 0;
        doc["wifi"]["reconnections"]  = counters.wifiReconnections;

        // MQTT
        doc["mqtt"]["enabled"]        = systemStatus.isHAAvailable;
        doc["mqtt"]["connected"]      = systemStatus.isHAAvailable && mqttHandler.isConnected();
        doc["mqtt"]["reconnections"]  = counters.mqttReconnections;

        // Sensores
        doc["sensors"]["bbqReadErrors"]      = counters.sensorBBQErrors;
        doc["sensors"]["proteinReadErrors"]  = counters.sensorPrtErrors;
        doc["sensors"]["internalReadErrors"] = counters.sensorIntErrors;

        // Tasks (high-watermark = mínimo de stack livre já visto)
        doc["tasks"]["tempStack"]    = stacks.tempTask;
        doc["tasks"]["controlStack"] = stacks.controlTask;
        doc["tasks"]["mqttStack"]    = stacks.mqttTask;

        // Relé
        doc["relay"]["emergencies"] = counters.relayEmergencies;

        ResponseHelper::sendJsonResponse(request, 200, "Diagnósticos obtidos com sucesso", doc.as<JsonObject>());
    });
}
