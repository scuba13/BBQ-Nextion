#include "MonitorEndpoints.h"
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>

MonitorEndpoints::MonitorEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l)
    : BaseEndpoint(s, ss, l) {}

void MonitorEndpoints::registerRoutes() {
    server.on(Routes::Monitor::BASE, HTTP_GET, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::Monitor::BASE, "GET");
        
        DynamicJsonDocument doc(1024);
        doc["currentTemp"] = systemStatus.calibratedTemp;
        doc["setTemp"] = systemStatus.bbqTemperature;
        doc["proteinTemp"] = systemStatus.calibratedTempP;
        doc["proteinTempSet"] = systemStatus.proteinTemperature;
        doc["relayState"] = systemStatus.isRelayOn ? "ON" : "OFF";
        
        ResponseHelper::sendJsonResponse(request, 200, "Monitor data fetched", doc.as<JsonObject>());
    });
}
