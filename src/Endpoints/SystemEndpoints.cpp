#include "SystemEndpoints.h"
#include "LogHandler.h"     // Inclua o novo LogHandler aqui
#include "ResponseHelper.h" // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>
#include <Nextion.h>
#include "OTAHandler.h" 
#include "TemperatureControl.h"

void registerSystemEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger, OTAHandler &otaHandler)
{
    server.on("/api/v1/system/activateCure", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        systemStatus.cureProcessMode = true;

        logger.logRequest(request, "Cure process activated");

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Cure process activated successfully");

        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Cure process activated successfully"); });

    // Novo endpoint para resetar o sistema
    server.on("/api/v1/system/reset", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        // Chama o método resetSystem passando o systemStatus como parâmetro
        resetSystem(systemStatus);

        logger.logRequest(request, "System reset initiated");

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "System reset successfully");

        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("System reset successfully"); });

    // Endpoint para atualização OTA
    server.on("/api/v1/system/update", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            // Primeiro callback - chamado após upload completo
            if (otaHandler.getStatus().inProgress) {
                if (otaHandler.endUpdate()) {
                    ResponseHelper::sendJsonResponse(request, 200, "Atualização concluída com sucesso");
                } else {
                    ResponseHelper::sendErrorResponse(request, 500, "Falha na atualização");
                }
            } else {
                ResponseHelper::sendErrorResponse(request, 400, "Nenhuma atualização em andamento");
            }
        },
        [&](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            // Segundo callback - chamado durante o upload
            if (!index) {
                // Início do upload
                String version = request->hasHeader("X-Firmware-Version") ? 
                               request->header("X-Firmware-Version") : "unknown";
                otaHandler.beginUpdate(request->contentLength(), version);
            }

            if (otaHandler.getStatus().inProgress) {
                if (!otaHandler.writeUpdate(data, len)) {
                    ResponseHelper::sendErrorResponse(request, 500, "Erro ao escrever firmware");
                    return;
                }
            }
        }
    );

    // Endpoint para verificar status da atualização
    server.on("/api/v1/system/update/status", HTTP_GET, [&](AsyncWebServerRequest *request) {
        auto status = otaHandler.getStatus();
        
        DynamicJsonDocument doc(1024);
        doc["inProgress"] = status.inProgress;
        doc["progress"] = status.progress;
        doc["currentVersion"] = status.currentVersion;
        doc["newVersion"] = status.newVersion;
        
        ResponseHelper::sendJsonResponse(request, 200, "Status da atualização", doc.as<JsonObject>());
    });

    // Endpoint para forçar rollback
    server.on("/api/v1/system/rollback", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (otaHandler.performRollback()) {
            ResponseHelper::sendJsonResponse(request, 200, "Rollback iniciado");
        } else {
            ResponseHelper::sendErrorResponse(request, 500, "Falha ao iniciar rollback");
        }
    });
}
