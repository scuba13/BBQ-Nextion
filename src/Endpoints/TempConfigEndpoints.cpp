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

    bool updated = false;

    if (request->hasParam("minBBQTemp", true)) {
        int newMinBBQTemp = request->getParam("minBBQTemp", true)->value().toInt();
        fileSystem.saveConfigToDatabase("minBBQTemp", String(newMinBBQTemp).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("maxBBQTemp", true)) {
        int newMaxBBQTemp = request->getParam("maxBBQTemp", true)->value().toInt();
        fileSystem.saveConfigToDatabase("maxBBQTemp", String(newMaxBBQTemp).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("minPrtTemp", true)) {
        int newMinPrtTemp = request->getParam("minPrtTemp", true)->value().toInt();
        fileSystem.saveConfigToDatabase("minPrtTemp", String(newMinPrtTemp).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("maxPrtTemp", true)) {
        int newMaxPrtTemp = request->getParam("maxPrtTemp", true)->value().toInt();
        fileSystem.saveConfigToDatabase("maxPrtTemp", String(newMaxPrtTemp).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("minCaliTemp", true)) {
        int newMinCaliTemp = request->getParam("minCaliTemp", true)->value().toInt();
        fileSystem.saveConfigToDatabase("minCaliTemp", String(newMinCaliTemp).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("maxCaliTemp", true)) {
        int newMaxCaliTemp = request->getParam("maxCaliTemp", true)->value().toInt();
        fileSystem.saveConfigToDatabase("maxCaliTemp", String(newMaxCaliTemp).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("minCaliTempP", true)) {
        int newMinCaliTempP = request->getParam("minCaliTempP", true)->value().toInt();
        fileSystem.saveConfigToDatabase("minCaliTempP", String(newMinCaliTempP).c_str(), systemStatus);
        updated = true;
    }

    if (request->hasParam("maxCaliTempP", true)) {
        int newMaxCaliTempP = request->getParam("maxCaliTempP", true)->value().toInt();
        fileSystem.saveConfigToDatabase("maxCaliTempP", String(newMaxCaliTempP).c_str(), systemStatus);
        updated = true;
    }

    if (!updated) {
        logger.logError("No valid parameters provided for update");
        ResponseHelper::sendErrorResponse(request, 400, "No valid parameters provided for update");
        return;
    }

    // Utilizando ResponseHelper para enviar a resposta
    ResponseHelper::sendJsonResponse(request, 200, "TempConfig configuration updated successfully");

    // Log da mensagem de sucesso utilizando o novo LogHandler
    logger.logMessage("TempConfig configuration updated successfully");
});

}
