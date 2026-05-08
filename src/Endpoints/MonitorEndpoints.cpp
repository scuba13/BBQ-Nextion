#include "Endpoints/MonitorEndpoints.h"
#include "Handlers/LogHandler.h"       // Inclua o novo LogHandler aqui
#include "Endpoints/ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>

void registerMonitorEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger)
{
    server.on("/api/v1/monitor", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request)
    {
        logger.logRequest(request, "Buscando dados de monitoramento");

        JsonDocument doc;
        doc["bbqCurrentTemp"] = systemStatus.calibratedTemp;
        doc["bbqSetpoint"] = systemStatus.bbqTemperature;
        doc["proteinCurrentTemp"] = systemStatus.calibratedTempP;
        doc["proteinSetpoint"] = systemStatus.proteinTemperature;
        doc["relayState"] = systemStatus.isRelayOn ? "ON" : "OFF";
        doc["avgTemp"] = systemStatus.averageTemp;
        doc["caliTemp"] = systemStatus.tempCalibration;
        doc["caliTempP"] = systemStatus.tempCalibrationP;
        doc["minBBQTemp"] = systemStatus.minBBQTemp;
        doc["maxBBQTemp"] = systemStatus.maxBBQTemp;
        doc["minPrtTemp"] = systemStatus.minPrtTemp;
        doc["maxPrtTemp"] = systemStatus.maxPrtTemp;
        doc["minCaliTemp"] = systemStatus.minCaliTemp;
        doc["maxCaliTemp"] = systemStatus.maxCaliTemp;
        doc["minCaliTempP"] = systemStatus.minCaliTempP;
        doc["maxCaliTempP"] = systemStatus.maxCaliTempP;
        doc["internalTemp"]    = systemStatus.calibratedTempInternal;
        doc["proteinReached"]  = systemStatus.proteinReached;

        ResponseHelper::sendJsonResponse(request, 200, "Dados de monitoramento obtidos com sucesso", doc.as<JsonObject>());
    });
}
