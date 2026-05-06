#include "TempConfigEndpoints.h"
#include "LogHandler.h"
#include "ResponseHelper.h"
#include <ArduinoJson.h>

void registerTempConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger) {
    server.on("/api/v1/temp/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando configuração de limites de temperatura");

        JsonDocument doc;
        doc["minBBQTemp"] = systemStatus.minBBQTemp;
        doc["maxBBQTemp"] = systemStatus.maxBBQTemp;
        doc["minPrtTemp"] = systemStatus.minPrtTemp;
        doc["maxPrtTemp"] = systemStatus.maxPrtTemp;
        doc["minCaliTemp"] = systemStatus.minCaliTemp;
        doc["maxCaliTemp"] = systemStatus.maxCaliTemp;
        doc["minCaliTempP"] = systemStatus.minCaliTempP;
        doc["maxCaliTempP"] = systemStatus.maxCaliTempP;

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de limites obtida com sucesso", doc.as<JsonObject>());
    });

    server.on("/api/v1/temp/config", HTTP_PATCH, [&systemStatus, &fileSystem, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Atualizando configuração de limites de temperatura");

        int newMinBBQTemp = systemStatus.minBBQTemp;
        int newMaxBBQTemp = systemStatus.maxBBQTemp;
        int newMinPrtTemp = systemStatus.minPrtTemp;
        int newMaxPrtTemp = systemStatus.maxPrtTemp;
        int newMinCaliTemp = systemStatus.minCaliTemp;
        int newMaxCaliTemp = systemStatus.maxCaliTemp;
        int newMinCaliTempP = systemStatus.minCaliTempP;
        int newMaxCaliTempP = systemStatus.maxCaliTempP;

        bool updated = false;

        if (request->hasParam("minBBQTemp", true)) { newMinBBQTemp = request->getParam("minBBQTemp", true)->value().toInt(); updated = true; }
        if (request->hasParam("maxBBQTemp", true)) { newMaxBBQTemp = request->getParam("maxBBQTemp", true)->value().toInt(); updated = true; }
        if (request->hasParam("minPrtTemp", true)) { newMinPrtTemp = request->getParam("minPrtTemp", true)->value().toInt(); updated = true; }
        if (request->hasParam("maxPrtTemp", true)) { newMaxPrtTemp = request->getParam("maxPrtTemp", true)->value().toInt(); updated = true; }
        if (request->hasParam("minCaliTemp", true)) { newMinCaliTemp = request->getParam("minCaliTemp", true)->value().toInt(); updated = true; }
        if (request->hasParam("maxCaliTemp", true)) { newMaxCaliTemp = request->getParam("maxCaliTemp", true)->value().toInt(); updated = true; }
        if (request->hasParam("minCaliTempP", true)) { newMinCaliTempP = request->getParam("minCaliTempP", true)->value().toInt(); updated = true; }
        if (request->hasParam("maxCaliTempP", true)) { newMaxCaliTempP = request->getParam("maxCaliTempP", true)->value().toInt(); updated = true; }

        if (!updated) {
            ResponseHelper::sendErrorResponse(request, 400, "Nenhum parâmetro válido fornecido para atualização");
            return;
        }

        if (newMinBBQTemp >= newMaxBBQTemp || newMinPrtTemp >= newMaxPrtTemp ||
            newMinCaliTemp >= newMaxCaliTemp || newMinCaliTempP >= newMaxCaliTempP) {
            ResponseHelper::sendErrorResponse(request, 400, "Valor mínimo deve ser menor que o máximo");
            return;
        }

        systemStatus.minBBQTemp = newMinBBQTemp;
        systemStatus.maxBBQTemp = newMaxBBQTemp;
        systemStatus.minPrtTemp = newMinPrtTemp;
        systemStatus.maxPrtTemp = newMaxPrtTemp;
        systemStatus.minCaliTemp = newMinCaliTemp;
        systemStatus.maxCaliTemp = newMaxCaliTemp;
        systemStatus.minCaliTempP = newMinCaliTempP;
        systemStatus.maxCaliTempP = newMaxCaliTempP;

        fileSystem.saveConfigToFile(systemStatus);

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de limites atualizada com sucesso");
    });
}
