#include "AIEndpoints.h"
#include <ArduinoJson.h>
#include <Nextion.h>


void registerAIEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, FileSystem& fileSystem) {
    server.on("/getAiConfig", HTTP_GET, [&systemStatus](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["aiKey"] = systemStatus.aiKey;
        doc["tip"] = systemStatus.tip;

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/updateAiConfig", HTTP_POST, [&systemStatus, &logger, &fileSystem](AsyncWebServerRequest *request) {
        logger.logMessage("Updating AI configuration");
        dbSerial.println("Updating AI configuration");

        String aiKey;
        String tip;

        if (request->hasParam("aiKey", true)) {
            aiKey = request->getParam("aiKey", true)->value();
        } else {
            request->send(400, "application/json", "{ \"error\": \"aiKey parameter is missing\" }");
            return;
        }

        if (request->hasParam("tip", true)) {
            tip = request->getParam("tip", true)->value();
        } else {
            request->send(400, "application/json", "{ \"error\": \"tip parameter is missing\" }");
            return;
        }

        strncpy(systemStatus.aiKey, aiKey.c_str(), sizeof(systemStatus.aiKey) - 1);
        systemStatus.aiKey[sizeof(systemStatus.aiKey) - 1] = '\0';

        strncpy(systemStatus.tip, tip.c_str(), sizeof(systemStatus.tip) - 1);
        systemStatus.tip[sizeof(systemStatus.tip) - 1] = '\0';

        fileSystem.saveConfigToFile(systemStatus);

        logger.logMessage("AI configuration updated");
        dbSerial.println("AI configuration updated");
        request->send(200, "application/json", "{ \"status\": \"success\" }");
    });
}
