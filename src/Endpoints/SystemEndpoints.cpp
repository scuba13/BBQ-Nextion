#include "SystemEndpoints.h"
#include "LogHandler.h"
#include "ResponseHelper.h"
#include <ArduinoJson.h>
#include "OTAHandler.h"
#include "TemperatureControl.h"

void registerSystemEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger, OTAHandler &otaHandler)
{
    server.on("/api/v1/system/activateCure", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        systemStatus.cureProcessMode = true;

        logger.logRequest(request, "Ativando modo de cura");

        ResponseHelper::sendJsonResponse(request, 200, "Modo de cura ativado com sucesso"); });

    // Novo endpoint para resetar o sistema
    server.on("/api/v1/system/reset", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        resetSystem(systemStatus);

        logger.logRequest(request, "Reset do sistema iniciado");

        ResponseHelper::sendJsonResponse(request, 200, "Sistema resetado com sucesso"); });

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
        
        JsonDocument doc;
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

    // Adicionar novo endpoint
    server.on("/api/v1/system/update/check", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!request->hasParam("version", true)) {
            ResponseHelper::sendErrorResponse(request, 400, "Versão não especificada");
            return;
        }
        
        String newVersion = request->getParam("version", true)->value();
        
        JsonDocument doc;
        doc["currentVersion"] = otaHandler.getFirmwareVersion();
        doc["canUpdate"] = otaHandler.hasEnoughSpace() && otaHandler.isVersionNewer(newVersion);
        doc["freeHeap"] = ESP.getFreeHeap();
        
        ResponseHelper::sendJsonResponse(request, 200, "Verificação concluída", doc.as<JsonObject>());
    });
}
