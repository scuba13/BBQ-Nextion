#include "TemperatureEndpoints.h"
#include "LogHandler.h"      // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"  // Inclua o ResponseHelper aqui

void registerTemperatureEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/api/v1/temperature/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching temperature config");

        DynamicJsonDocument data(1024);
        data["minBBQTemp"] = systemStatus.minBBQTemp;
        data["maxBBQTemp"] = systemStatus.maxBBQTemp;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Temperature config fetched successfully", data.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Temperature config fetched successfully");
    });

    server.on("/api/v1/temperature/config", HTTP_POST, [&](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Updating temperature config");

        if (request->hasParam("minBBQTemp", true)) {
            AsyncWebParameter* tempParam = request->getParam("minBBQTemp", true);
            systemStatus.minBBQTemp = tempParam->value().toInt();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Missing 'minBBQTemp' parameter");
            // Log de erro utilizando o novo LogHandler
            logger.logError("Missing 'minBBQTemp' parameter");
            return;
        }

        if (request->hasParam("maxBBQTemp", true)) {
            AsyncWebParameter* tempParam = request->getParam("maxBBQTemp", true);
            systemStatus.maxBBQTemp = tempParam->value().toInt();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Missing 'maxBBQTemp' parameter");
            // Log de erro utilizando o novo LogHandler
            logger.logError("Missing 'maxBBQTemp' parameter");
            return;
        }

        ResponseHelper::sendJsonResponse(request, 200, "Temperature config updated successfully");
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Temperature config updated successfully");
    });
}
