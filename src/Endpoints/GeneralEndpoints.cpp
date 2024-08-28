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

        File logFile = LittleFS.open("/log.txt", "r");
        if (!logFile) {
            logger.logError("Log file not found");
            ResponseHelper::sendErrorResponse(request, 404, "Log file not found");
            return;
        }

        String logContent = "";
        while (logFile.available()) {
            logContent += logFile.readStringUntil('\n') + '\n';
        }
        logFile.close();

        // Enviando o conteúdo do log
        request->send(200, "text/plain", logContent);
        logger.logMessage("Log content fetched successfully");
    });

    server.on("/api/v1/system/reset", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        resetSystem(systemStatus);

        logger.logMessage("System reset");

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "System reset successfully");

        logger.logMessage("System reset successfully");
    });

    server.on("/api/v1/firmware/update", HTTP_POST, [](AsyncWebServerRequest *request) {},
        [&otaHandler, &logger](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            otaHandler.handleFirmwareUpload(request, filename, index, data, len, final);
            if (final) {
                logger.logMessage("Firmware update completed successfully: " + filename);
            }
        });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}
