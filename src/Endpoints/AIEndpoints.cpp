#include "AIEndpoints.h"
#include <ArduinoJson.h>
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <Nextion.h>

AIEndpoints::AIEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l, FileSystem& fs)
    : BaseEndpoint(s, ss, l), fileSystem(fs) {}

void AIEndpoints::registerRoutes() {
    server.on(Routes::AI::CONFIG, HTTP_GET, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::AI::CONFIG, "GET");
        
        DynamicJsonDocument doc(1024);
        doc["aiKey"] = systemStatus.aiKey;
        doc["tip"] = systemStatus.tip;
        
        ResponseHelper::sendJsonResponse(request, 200, "AI configuration fetched", doc.as<JsonObject>());
    });

    server.on(Routes::AI::CONFIG, HTTP_PATCH, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::AI::CONFIG, "PATCH");
        
        if (!validateParam(request, "aiKey") || !validateParam(request, "tip")) {
            return;
        }

        String aiKey = request->getParam("aiKey", true)->value();
        String tip = request->getParam("tip", true)->value();

        strncpy(systemStatus.aiKey, aiKey.c_str(), sizeof(systemStatus.aiKey) - 1);
        strncpy(systemStatus.tip, tip.c_str(), sizeof(systemStatus.tip) - 1);
        
        fileSystem.saveConfigToFile(systemStatus);
        ResponseHelper::sendJsonResponse(request, 200, "AI configuration updated");
    });
}
