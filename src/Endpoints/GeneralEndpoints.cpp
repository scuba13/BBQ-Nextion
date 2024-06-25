#include "GeneralEndpoints.h"
#include <LittleFS.h>
#include <FS.h>
#include "TemperatureControl.h"

void handleGetLogContent(AsyncWebServerRequest *request);

void registerGeneralEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, OTAHandler& otaHandler) {
    server.on("/getLogContent", HTTP_GET, [](AsyncWebServerRequest *request) {
        File logFile = LittleFS.open("/log.txt", "r");
        if (!logFile) {
            request->send(404, "text/plain", "Arquivo de log não encontrado");
            return;
        }

        String logContent = "";
        while (logFile.available()) {
            logContent += logFile.readStringUntil('\n') + '\n';
        }
        logFile.close();

        request->send(200, "text/plain", logContent);
    });

    server.on("/resetSystem", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        resetSystem(systemStatus);
        Serial.println("Sistema resetado");
        logger.logMessage("Sistema resetado");
        request->send(200, "application/json", "{ \"status\": \"success\" }");
    });

    server.on("/updateFirmware", HTTP_POST, [](AsyncWebServerRequest *request) {},
        [&otaHandler](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            otaHandler.handleFirmwareUpload(request, filename, index, data, len, final);
        });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}
