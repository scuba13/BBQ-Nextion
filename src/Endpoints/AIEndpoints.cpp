#include "AIEndpoints.h"
#include <ArduinoJson.h>
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <Nextion.h>

void registerAIEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, FileSystem& fileSystem) {
    server.on("/api/v1/ai/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching AI configuration");

        DynamicJsonDocument doc(1024);
        doc["aiKey"] = systemStatus.aiKey;
        doc["tip"] = systemStatus.tip;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "AI configuration fetched successfully", doc.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("AI configuration fetched successfully");
    });

    server.on("/api/v1/ai/config", HTTP_PATCH, [&systemStatus, &logger, &fileSystem](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Updating AI configuration");

        String aiKey;
        String tip;

        if (request->hasParam("aiKey", true)) {
            aiKey = request->getParam("aiKey", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "aiKey parameter is missing");
            // Log de erro utilizando o novo LogHandler
            logger.logError("Missing 'aiKey' parameter");
            return;
        }

        if (request->hasParam("tip", true)) {
            tip = request->getParam("tip", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "tip parameter is missing");
            // Log de erro utilizando o novo LogHandler
            logger.logError("Missing 'tip' parameter");
            return;
        }

        // Atualiza os valores em systemStatus
        strncpy(systemStatus.aiKey, aiKey.c_str(), sizeof(systemStatus.aiKey) - 1);
        systemStatus.aiKey[sizeof(systemStatus.aiKey) - 1] = '\0';

        strncpy(systemStatus.tip, tip.c_str(), sizeof(systemStatus.tip) - 1);
        systemStatus.tip[sizeof(systemStatus.tip) - 1] = '\0';

        fileSystem.saveConfigToFile(systemStatus);

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "AI configuration updated successfully");
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("AI configuration updated successfully");
    });
}
