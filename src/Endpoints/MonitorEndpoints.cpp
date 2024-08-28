#include "MonitorEndpoints.h"
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>

void registerMonitorEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger)
{
    server.on("/api/v1/monitor", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request)
    {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching monitoring data");

        DynamicJsonDocument doc(1024); // Ajuste o tamanho conforme necessário
        doc["currentTemp"] = systemStatus.calibratedTemp;
        doc["setTemp"] = systemStatus.bbqTemperature;
        doc["proteinTemp"] = systemStatus.calibratedTempP;
        doc["proteinTempSet"] = systemStatus.proteinTemperature;
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
        doc["internalTemp"] = systemStatus.calibratedTempInternal;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Monitoring data fetched successfully", doc.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Monitoring data fetched successfully");
    });
}
