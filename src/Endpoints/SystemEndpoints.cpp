#include "Endpoints/SystemEndpoints.h"
#include "Handlers/LogHandler.h"
#include "Endpoints/ResponseHelper.h"
#include "SysStatMutex.h"
#include <ArduinoJson.h>
#include "Handlers/OTAHandler.h"
#include "Handlers/TemperatureHandler.h"

void registerSystemEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger, OTAHandler &otaHandler)
{
    server.on("/api/v1/system/reset", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
            ResponseHelper::sendUnauthorized(request);
            return;
        }
        resetSystem(systemStatus);
        logger.logRequest(request, "Reset do sistema iniciado");
        ResponseHelper::sendJsonResponse(request, 200, "Sistema resetado com sucesso"); });

    // Endpoint para atualização OTA
    server.on("/api/v1/system/update", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
                ResponseHelper::sendUnauthorized(request);
                return;
            }
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
            if (!ResponseHelper::isAuthenticated(request, systemStatus)) return;
            if (!index) {
                String version = request->hasHeader("X-Firmware-Version") ?
                               request->header("X-Firmware-Version") : "unknown";
                String md5 = request->hasHeader("X-Firmware-MD5") ?
                               request->header("X-Firmware-MD5") : "";
                otaHandler.beginUpdate(request->contentLength(), version, md5);
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
        if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
            ResponseHelper::sendUnauthorized(request);
            return;
        }
        if (otaHandler.performRollback()) {
            ResponseHelper::sendJsonResponse(request, 200, "Rollback iniciado");
        } else {
            ResponseHelper::sendErrorResponse(request, 500, "Falha ao iniciar rollback");
        }
    });

    server.on("/api/v1/system/update/check", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
            ResponseHelper::sendUnauthorized(request);
            return;
        }
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
