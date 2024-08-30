#include "TemperatureEndpoints.h"
#include "LogHandler.h"      // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"  // Inclua o ResponseHelper aqui

void registerTemperatureEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/api/v1/temperature/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching temperature config");

        DynamicJsonDocument data(1024);
        data["bbqTemperature"] = systemStatus.bbqTemperature;
        data["proteinTemperature"] = systemStatus.proteinTemperature;
        data["tempCalibration"] = systemStatus.tempCalibration;
        data["tempCalibrationP"] = systemStatus.tempCalibrationP;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Temperature config fetched successfully", data.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Temperature config fetched successfully");
    });

    server.on("/api/v1/temperature/config", HTTP_PATCH, [&](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Updating temperature config");

        bool updated = false;

        if (request->hasParam("bbqTemperature", true)) {
            systemStatus.bbqTemperature = request->getParam("bbqTemperature", true)->value().toFloat();
            updated = true;
        }

        if (request->hasParam("proteinTemperature", true)) {
            systemStatus.proteinTemperature = request->getParam("proteinTemperature", true)->value().toFloat();
            updated = true;
        }

        if (request->hasParam("tempCalibration", true)) {
            systemStatus.tempCalibration = request->getParam("tempCalibration", true)->value().toFloat();
            updated = true;
        }

        if (request->hasParam("tempCalibrationP", true)) {
            systemStatus.tempCalibrationP = request->getParam("tempCalibrationP", true)->value().toFloat();
            updated = true;
        }

        if (!updated) {
            ResponseHelper::sendErrorResponse(request, 400, "No valid parameters provided for update");
            logger.logError("No valid parameters provided for update");
            return;
        }

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Temperature config updated successfully");
        logger.logMessage("Temperature config updated successfully");
    });
}
