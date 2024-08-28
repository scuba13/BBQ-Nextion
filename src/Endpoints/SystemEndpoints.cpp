#include "SystemEndpoints.h"
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>
#include <Nextion.h>

void registerSystemEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/api/v1/system/tempCalibration", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        if (request->hasParam("tempCalibration", true)) {
            int tempCalibration = request->getParam("tempCalibration", true)->value().toInt();
            systemStatus.tempCalibration = tempCalibration;

            logger.logRequest(request, "Temp Calibration set to: " + String(systemStatus.tempCalibration));

            // Utilizando ResponseHelper para enviar a resposta
            ResponseHelper::sendJsonResponse(request, 200, "Temp Calibration updated successfully");

            // Log da mensagem de sucesso utilizando o novo LogHandler
            logger.logMessage("Temp Calibration updated successfully to: " + String(systemStatus.tempCalibration));
        } else {
            logger.logError("Parameter 'tempCalibration' not found in the request");
            ResponseHelper::sendErrorResponse(request, 400, "Parameter 'tempCalibration' not found in the request");
        }
    });

    server.on("/api/v1/system/tempCalibrationP", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        if (request->hasParam("tempCalibrationP", true)) {
            int tempCalibrationP = request->getParam("tempCalibrationP", true)->value().toInt();
            systemStatus.tempCalibrationP = tempCalibrationP;

            logger.logRequest(request, "Temp CalibrationP set to: " + String(systemStatus.tempCalibrationP));

            // Utilizando ResponseHelper para enviar a resposta
            ResponseHelper::sendJsonResponse(request, 200, "Temp CalibrationP updated successfully");

            // Log da mensagem de sucesso utilizando o novo LogHandler
            logger.logMessage("Temp CalibrationP updated successfully to: " + String(systemStatus.tempCalibrationP));
        } else {
            logger.logError("Parameter 'tempCalibrationP' not found in the request");
            ResponseHelper::sendErrorResponse(request, 400, "Parameter 'tempCalibrationP' not found in the request");
        }
    });

    server.on("/api/v1/system/activateCure", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        systemStatus.cureProcessMode = true;

        logger.logRequest(request, "Cure process activated");

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Cure process activated successfully");

        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Cure process activated successfully");
    });
}
