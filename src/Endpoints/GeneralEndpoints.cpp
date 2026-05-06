#include "GeneralEndpoints.h"
#include <LittleFS.h>
#include <FS.h>
#include "LogHandler.h"
#include "ResponseHelper.h"
#include "TemperatureControl.h"

void registerGeneralEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, OTAHandler& otaHandler) {
    server.on("/api/v1/log/content", HTTP_GET, [&logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando conteúdo do log");

        File logFile = LittleFS.open("/log.txt", "r");
        if (!logFile) {
            ResponseHelper::sendErrorResponse(request, 404, "Arquivo de log não encontrado");
            return;
        }

        std::vector<String> lines;
        while (logFile.available()) {
            lines.push_back(logFile.readStringUntil('\n'));
        }
        logFile.close();

        String logContent = "";
        int startLine = lines.size() > 100 ? lines.size() - 100 : 0;
        for (int i = startLine; i < (int)lines.size(); i++) {
            logContent += lines[i] + '\n';
        }

        if (logContent.length() == 0) {
            ResponseHelper::sendErrorResponse(request, 500, "Conteúdo do log está vazio");
            return;
        }

        JsonDocument doc;
        doc["logContent"] = logContent;

        ResponseHelper::sendJsonResponse(request, 200, "Conteúdo do log obtido com sucesso", doc.as<JsonObject>());
    });
}
