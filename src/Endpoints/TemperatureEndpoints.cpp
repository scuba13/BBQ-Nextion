#include "Endpoints/TemperatureEndpoints.h"
#include "Handlers/LogHandler.h"
#include "Handlers/FileSystem.h"
#include "Endpoints/ResponseHelper.h"
#include "SysStatMutex.h"

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
        if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
            ResponseHelper::sendUnauthorized(request);
            return;
        }
        logger.logRequest(request, "Atualizando configuração de temperatura");

        bool updated = false;

        // Valida os valores antes de escrever (leitura de limites é segura fora do lock)
        int newBBQTemp = -1, newPrtTemp = -1, newCali = -9999, newCaliP = -9999;

        if (request->hasParam("bbqTemperature", true)) {
            newBBQTemp = request->getParam("bbqTemperature", true)->value().toInt();
            if (newBBQTemp < systemStatus.minBBQTemp || newBBQTemp > systemStatus.maxBBQTemp) {
                ResponseHelper::sendErrorResponse(request, 400, "Temperatura BBQ fora do intervalo permitido");
                return;
            }
            updated = true;
        }
        if (request->hasParam("proteinTemperature", true)) {
            newPrtTemp = request->getParam("proteinTemperature", true)->value().toInt();
            if (newPrtTemp < systemStatus.minPrtTemp || newPrtTemp > systemStatus.maxPrtTemp) {
                ResponseHelper::sendErrorResponse(request, 400, "Temperatura da proteína fora do intervalo permitido");
                return;
            }
            updated = true;
        }
        if (request->hasParam("tempCalibration", true)) {
            newCali = request->getParam("tempCalibration", true)->value().toInt();
            if (newCali < systemStatus.minCaliTemp || newCali > systemStatus.maxCaliTemp) {
                ResponseHelper::sendErrorResponse(request, 400, "Calibração BBQ fora do intervalo permitido");
                return;
            }
            updated = true;
        }
        if (request->hasParam("tempCalibrationP", true)) {
            newCaliP = request->getParam("tempCalibrationP", true)->value().toInt();
            if (newCaliP < systemStatus.minCaliTempP || newCaliP > systemStatus.maxCaliTempP) {
                ResponseHelper::sendErrorResponse(request, 400, "Calibração da proteína fora do intervalo permitido");
                return;
            }
            updated = true;
        }

        if (!updated) {
            ResponseHelper::sendErrorResponse(request, 400, "Nenhum parâmetro válido fornecido para atualização");
            return;
        }

        // Aplica as escritas sob mutex
        sysStatLock();
        if (newBBQTemp  != -1)    systemStatus.bbqTemperature    = newBBQTemp;
        if (newPrtTemp  != -1)    systemStatus.proteinTemperature = newPrtTemp;
        if (newCali     != -9999) systemStatus.tempCalibration    = newCali;
        if (newCaliP    != -9999) systemStatus.tempCalibrationP   = newCaliP;
        sysStatUnlock();

        // C-01: persiste setpoints e calibração (campos que faltavam no save anterior)
        if (!FileSystem::saveConfigToFile(systemStatus)) {
            ResponseHelper::sendErrorResponse(request, 500, "Falha ao salvar configuração");
            return;
        }

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de temperatura atualizada com sucesso");
    });
}
