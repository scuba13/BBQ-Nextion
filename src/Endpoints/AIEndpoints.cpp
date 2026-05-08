#include "Endpoints/AIEndpoints.h"
#include <ArduinoJson.h>
#include "Handlers/LogHandler.h"
#include "Endpoints/ResponseHelper.h"
#include "SysStatMutex.h"

void registerAIEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, FileSystem& fileSystem) {
    server.on("/api/v1/ai/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando configuração de IA");

        JsonDocument doc;
        doc["aiKey"] = systemStatus.aiKey;
        doc["tip"] = systemStatus.tip;

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de IA obtida com sucesso", doc.as<JsonObject>());
    });

    server.on("/api/v1/ai/config", HTTP_PATCH, [&systemStatus, &logger, &fileSystem](AsyncWebServerRequest *request) {
        if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
            ResponseHelper::sendUnauthorized(request);
            return;
        }
        logger.logRequest(request, "Atualizando configuração de IA");

        String aiKey;
        String tip;

        if (request->hasParam("aiKey", true)) {
            aiKey = request->getParam("aiKey", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'aiKey' ausente");
            return;
        }

        if (request->hasParam("tip", true)) {
            tip = request->getParam("tip", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'tip' ausente");
            return;
        }

        sysStatLock();
        strncpy(systemStatus.aiKey, aiKey.c_str(), sizeof(systemStatus.aiKey) - 1);
        systemStatus.aiKey[sizeof(systemStatus.aiKey) - 1] = '\0';
        strncpy(systemStatus.tip, tip.c_str(), sizeof(systemStatus.tip) - 1);
        systemStatus.tip[sizeof(systemStatus.tip) - 1] = '\0';
        sysStatUnlock();

        fileSystem.saveConfigToFile(systemStatus);

        ResponseHelper::sendJsonResponse(request, 200, "Configuração de IA atualizada com sucesso");
    });
}
