#include "Endpoints/GeneralEndpoints.h"
#include <LittleFS.h>
#include <FS.h>
#include "Handlers/LogHandler.h"
#include "Endpoints/ResponseHelper.h"
#include "Handlers/TemperatureHandler.h"

// A-02: tamanho máximo de log enviado ao cliente (evita OOM com arquivo de 50KB)
static const size_t LOG_TAIL_BYTES = 5000;

void registerGeneralEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, OTAHandler& otaHandler) {
    server.on("/api/v1/log/content", HTTP_GET, [&logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando conteúdo do log");

        File logFile = LittleFS.open("/log.txt", "r");
        if (!logFile) {
            ResponseHelper::sendErrorResponse(request, 404, "Arquivo de log não encontrado");
            return;
        }

        // Lê apenas os últimos LOG_TAIL_BYTES — sem vector, sem concatenação O(n²)
        size_t fileSize = logFile.size();
        if (fileSize > LOG_TAIL_BYTES) logFile.seek(fileSize - LOG_TAIL_BYTES);
        String logContent = logFile.readString();
        logFile.close();

        if (logContent.isEmpty()) {
            ResponseHelper::sendErrorResponse(request, 500, "Conteúdo do log está vazio");
            return;
        }

        JsonDocument doc;
        doc["logContent"] = logContent;
        ResponseHelper::sendJsonResponse(request, 200, "Conteúdo do log obtido com sucesso", doc.as<JsonObject>());
    });
}
