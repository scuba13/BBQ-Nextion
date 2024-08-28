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
        int startLine = lines.size() > 200 ? lines.size() - 200 : 0;
        for (int i = startLine; i < lines.size(); i++) {
            logContent += lines[i] + '\n';
        }

        // Verificação se o conteúdo foi lido corretamente
        if (logContent.length() == 0) {
            logger.logError("Log content is empty after reading the file.");
            ResponseHelper::sendErrorResponse(request, 500, "Log content is empty after reading the file.");
            return;
        }

        // Enviando o conteúdo do log (últimas 100 linhas)
        request->send(200, "text/plain", logContent);
        logger.logMessage("Log content fetched successfully");
    });
}
