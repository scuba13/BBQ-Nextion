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

    server.on("/api/v1/temperature/config", HTTP_POST, [&](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Updating temperature config");

        if (request->hasParam("bbqTemperature", true)) {
            AsyncWebParameter* tempParam = request->getParam("bbqTemperature", true);
            systemStatus.bbqTemperature = tempParam->value().toFloat();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Missing 'bbqTemperature' parameter");
            logger.logError("Missing 'bbqTemperature' parameter");
            return;
        }

        if (request->hasParam("proteinTemperature", true)) {
            AsyncWebParameter* tempParam = request->getParam("proteinTemperature", true);
            systemStatus.proteinTemperature = tempParam->value().toFloat();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Missing 'proteinTemperature' parameter");
            logger.logError("Missing 'proteinTemperature' parameter");
            return;
        }

        if (request->hasParam("tempCalibration", true)) {
            AsyncWebParameter* tempParam = request->getParam("tempCalibration", true);
            systemStatus.tempCalibration = tempParam->value().toFloat();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Missing 'tempCalibration' parameter");
            logger.logError("Missing 'tempCalibration' parameter");
            return;
        }

        if (request->hasParam("tempCalibrationP", true)) {
            AsyncWebParameter* tempParam = request->getParam("tempCalibrationP", true);
            systemStatus.tempCalibrationP = tempParam->value().toFloat();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Missing 'tempCalibrationP' parameter");
            logger.logError("Missing 'tempCalibrationP' parameter");
            return;
        }

        ResponseHelper::sendJsonResponse(request, 200, "Temperature config updated successfully");
        logger.logMessage("Temperature config updated successfully");
    });
}
