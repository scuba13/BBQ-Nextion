#include "TempConfigEndpoints.h"
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>
#include <Nextion.h>

void registerTempConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger) {
    server.on("/api/v1/temp/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching TempConfig configuration");

        DynamicJsonDocument doc(1024);
        doc["minBBQTemp"] = systemStatus.minBBQTemp;
        doc["maxBBQTemp"] = systemStatus.maxBBQTemp;
        doc["minPrtTemp"] = systemStatus.minPrtTemp;
        doc["maxPrtTemp"] = systemStatus.maxPrtTemp;
        doc["minCaliTemp"] = systemStatus.minCaliTemp;
        doc["maxCaliTemp"] = systemStatus.maxCaliTemp;
        doc["minCaliTempP"] = systemStatus.minCaliTempP;
        doc["maxCaliTempP"] = systemStatus.maxCaliTempP;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "TempConfig configuration fetched successfully", doc.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("TempConfig configuration fetched successfully");
    });

    server.on("/api/v1/temp/config", HTTP_PATCH, [&systemStatus, &fileSystem, &logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Updating TempConfig configuration");

        int newMinBBQTemp = systemStatus.minBBQTemp;
        int newMaxBBQTemp = systemStatus.maxBBQTemp;
        int newMinPrtTemp = systemStatus.minPrtTemp;
        int newMaxPrtTemp = systemStatus.maxPrtTemp;
        int newMinCaliTemp = systemStatus.minCaliTemp;
        int newMaxCaliTemp = systemStatus.maxCaliTemp;
        int newMinCaliTempP = systemStatus.minCaliTempP;
        int newMaxCaliTempP = systemStatus.maxCaliTempP;

        bool updated = false;

        if (request->hasParam("minBBQTemp", true)) {
            newMinBBQTemp = request->getParam("minBBQTemp", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("maxBBQTemp", true)) {
            newMaxBBQTemp = request->getParam("maxBBQTemp", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("minPrtTemp", true)) {
            newMinPrtTemp = request->getParam("minPrtTemp", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("maxPrtTemp", true)) {
            newMaxPrtTemp = request->getParam("maxPrtTemp", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("minCaliTemp", true)) {
            newMinCaliTemp = request->getParam("minCaliTemp", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("maxCaliTemp", true)) {
            newMaxCaliTemp = request->getParam("maxCaliTemp", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("minCaliTempP", true)) {
            newMinCaliTempP = request->getParam("minCaliTempP", true)->value().toInt();
            updated = true;
        }

        if (request->hasParam("maxCaliTempP", true)) {
            newMaxCaliTempP = request->getParam("maxCaliTempP", true)->value().toInt();
            updated = true;
        }

        if (!updated) {
            logger.logError("No valid parameters provided for update");
            ResponseHelper::sendErrorResponse(request, 400, "No valid parameters provided for update");
            return;
        }

        // Atualiza as configurações no systemStatus
        systemStatus.minBBQTemp = newMinBBQTemp;
        systemStatus.maxBBQTemp = newMaxBBQTemp;
        systemStatus.minPrtTemp = newMinPrtTemp;
        systemStatus.maxPrtTemp = newMaxPrtTemp;
        systemStatus.minCaliTemp = newMinCaliTemp;
        systemStatus.maxCaliTemp = newMaxCaliTemp;
        systemStatus.minCaliTempP = newMinCaliTempP;
        systemStatus.maxCaliTempP = newMaxCaliTempP;

        // Salva a configuração no sistema de arquivos
        fileSystem.saveConfigToFile(systemStatus);

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "TempConfig configuration updated successfully");

        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("TempConfig configuration updated successfully");
    });
}
