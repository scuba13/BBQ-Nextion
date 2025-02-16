#include "EnergyEndpoints.h"
#include <ArduinoJson.h>
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <Nextion.h>

EnergyEndpoints::EnergyEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l)
    : BaseEndpoint(s, ss, l) {}

void EnergyEndpoints::registerRoutes() {
    // GET energy data
    server.on(Routes::Energy::BASE, HTTP_GET, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::Energy::BASE, "GET");
        
        DynamicJsonDocument doc(1024);
        doc["power"] = systemStatus.power;
        doc["energy"] = systemStatus.energy;
        doc["cost"] = systemStatus.cost;
        doc["kWhCost"] = systemStatus.kWhCost;
        
        ResponseHelper::sendJsonResponse(request, 200, "Energy data fetched", doc.as<JsonObject>());
    });

    // PATCH energy cost
    server.on(Routes::Energy::COST, HTTP_PATCH, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::Energy::COST, "PATCH");

        if (!validateParam(request, "kWhCost")) {
            return;
        }

        float kWhCost = request->getParam("kWhCost", true)->value().toFloat();
        systemStatus.kWhCost = kWhCost;

        DynamicJsonDocument doc(1024);
        doc["kWhCost"] = systemStatus.kWhCost;
        
        ResponseHelper::sendJsonResponse(request, 200, "Energy cost updated", doc.as<JsonObject>());
    });
}
