#include "AuthEndpoints.h"
#include "ResponseHelper.h"
#include "SysStatMutex.h"
#include <ArduinoJson.h>

void registerAuthEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger) {

    // GET: retorna se a chave está configurada (sem revelar o valor)
    server.on("/api/v1/auth/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando status de autenticação");

        JsonDocument doc;
        bool configured = strlen(systemStatus.apiKey) > 0;
        doc["keyConfigured"] = configured;
        doc["keyPreview"] = configured
            ? (String(systemStatus.apiKey).substring(0, 4) + "****")
            : "";

        ResponseHelper::sendJsonResponse(request, 200, "Status de autenticação obtido", doc.as<JsonObject>());
    });

    // PATCH: define ou altera a API key
    // - Se não há chave configurada: aceita qualquer newKey
    // - Se há chave: exige currentKey correto para trocar
    server.on("/api/v1/auth/config", HTTP_PATCH, [&systemStatus, &fileSystem, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Atualizando chave de API");

        bool hasKey = strlen(systemStatus.apiKey) > 0;

        if (hasKey) {
            if (!request->hasParam("currentKey", true)) {
                ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'currentKey' ausente");
                return;
            }
            String currentKey = request->getParam("currentKey", true)->value();
            if (currentKey != systemStatus.apiKey) {
                ResponseHelper::sendErrorResponse(request, 401, "Chave atual incorreta");
                return;
            }
        }

        if (!request->hasParam("newKey", true)) {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'newKey' ausente");
            return;
        }

        String newKey = request->getParam("newKey", true)->value();
        if (newKey.length() > 32) {
            ResponseHelper::sendErrorResponse(request, 400, "Chave deve ter no máximo 32 caracteres");
            return;
        }

        sysStatLock();
        strncpy(systemStatus.apiKey, newKey.c_str(), sizeof(systemStatus.apiKey) - 1);
        systemStatus.apiKey[sizeof(systemStatus.apiKey) - 1] = '\0';
        sysStatUnlock();

        fileSystem.saveConfigToFile(systemStatus);

        ResponseHelper::sendJsonResponse(request, 200, newKey.length() == 0
            ? "Autenticação desabilitada com sucesso"
            : "Chave de API atualizada com sucesso");
    });
}
