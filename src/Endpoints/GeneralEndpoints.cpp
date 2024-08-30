#include "GeneralEndpoints.h"
#include <LittleFS.h>
#include <FS.h>
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include "TemperatureControl.h"
#include <Nextion.h>

void registerGeneralEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, OTAHandler& otaHandler) {
    server.on("/api/v1/log/content", HTTP_GET, [&logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching log content");

        // Abrindo o arquivo de log
        File logFile = LittleFS.open("/log.txt", "r");
        if (!logFile) {
            logger.logError("Log file not found");
            ResponseHelper::sendErrorResponse(request, 404, "Log file not found");
            return;
        }

        // Lendo todas as linhas do arquivo
        std::vector<String> lines;
        while (logFile.available()) {
            String line = logFile.readStringUntil('\n');
            lines.push_back(line);
        }
        logFile.close();

        // Selecionando as últimas 100 linhas
        String logContent = "";
        int startLine = lines.size() > 100 ? lines.size() - 100 : 0;
        for (int i = startLine; i < lines.size(); i++) {
            logContent += lines[i] + '\n';
        }

        // Verificação se o conteúdo foi lido corretamente
        if (logContent.length() == 0) {
            logger.logError("Log content is empty after reading the file.");
            ResponseHelper::sendErrorResponse(request, 500, "Log content is empty after reading the file.");
            return;
        }

        // Criando um JsonObject para armazenar o logContent e passar para o ResponseHelper
        DynamicJsonDocument doc(4096);
        doc["logContent"] = logContent;

        // Enviando o conteúdo do log usando o ResponseHelper
        ResponseHelper::sendJsonResponse(request, 200, "Log content fetched successfully", doc.as<JsonObject>());
        logger.logMessage("Log content fetched successfully");
    });
}
