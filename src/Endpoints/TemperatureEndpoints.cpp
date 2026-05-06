#include "TemperatureEndpoints.h"
#include "LogHandler.h"      // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"  // Inclua o ResponseHelper aqui

void registerTemperatureEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/api/v1/temperature/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando configuração de temperatura");

        JsonDocument data;
        data["bbqTemperature"] = systemStatus.bbqTemperature;
        data["proteinTemperature"] = systemStatus.proteinTemperature;
        data["tempCalibration"] = systemStatus.tempCalibration;
        data["tempCalibrationP"] = systemStatus.tempCalibrationP;

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de temperatura obtida com sucesso", data.as<JsonObject>());
    });

    server.on("/api/v1/temperature/config", HTTP_PATCH, [&](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Atualizando configuração de temperatura");

        bool updated = false;

        if (request->hasParam("bbqTemperature", true)) {
            int newBBQTemp = request->getParam("bbqTemperature", true)->value().toInt();
            if (newBBQTemp < systemStatus.minBBQTemp || newBBQTemp > systemStatus.maxBBQTemp) {
                ResponseHelper::sendErrorResponse(request, 400, "Temperatura BBQ fora do intervalo permitido");
                return;
            }
            systemStatus.bbqTemperature = newBBQTemp;
            updated = true;
        }

        if (request->hasParam("proteinTemperature", true)) {
            int newPrtTemp = request->getParam("proteinTemperature", true)->value().toInt();
            if (newPrtTemp < systemStatus.minPrtTemp || newPrtTemp > systemStatus.maxPrtTemp) {
                ResponseHelper::sendErrorResponse(request, 400, "Temperatura da proteína fora do intervalo permitido");
                return;
            }
            systemStatus.proteinTemperature = newPrtTemp;
            updated = true;
        }

        if (request->hasParam("tempCalibration", true)) {
            int newCali = request->getParam("tempCalibration", true)->value().toInt();
            if (newCali < systemStatus.minCaliTemp || newCali > systemStatus.maxCaliTemp) {
                ResponseHelper::sendErrorResponse(request, 400, "Calibração BBQ fora do intervalo permitido");
                return;
            }
            systemStatus.tempCalibration = newCali;
            updated = true;
        }

        if (request->hasParam("tempCalibrationP", true)) {
            int newCaliP = request->getParam("tempCalibrationP", true)->value().toInt();
            if (newCaliP < systemStatus.minCaliTempP || newCaliP > systemStatus.maxCaliTempP) {
                ResponseHelper::sendErrorResponse(request, 400, "Calibração da proteína fora do intervalo permitido");
                return;
            }
            systemStatus.tempCalibrationP = newCaliP;
            updated = true;
        }

        if (!updated) {
            ResponseHelper::sendErrorResponse(request, 400, "Nenhum parâmetro válido fornecido para atualização");
            return;
        }

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de temperatura atualizada com sucesso");
    });
}
