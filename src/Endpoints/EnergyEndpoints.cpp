#include "EnergyEndpoints.h"
#include <ArduinoJson.h>
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <Nextion.h>

void registerEnergyEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/api/v1/energy", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando dados de energia");

        JsonDocument doc;
        doc["power"] = systemStatus.power;
        doc["energy"] = systemStatus.energy;
        doc["cost"] = systemStatus.cost;
        doc["kWhCost"] = systemStatus.kWhCost;

        ResponseHelper::sendJsonResponse(request, 200, "Dados de energia obtidos com sucesso", doc.as<JsonObject>());
    });

    server.on("/api/v1/energy/cost", HTTP_PATCH, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Atualizando custo de energia");

        if (request->hasParam("kWhCost", true)) {
            systemStatus.kWhCost = request->getParam("kWhCost", true)->value().toFloat();

            JsonDocument jsonDoc;
            jsonDoc["kWhCost"] = systemStatus.kWhCost;
            ResponseHelper::sendJsonResponse(request, 200, "Custo de energia atualizado com sucesso", jsonDoc.as<JsonObject>());
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'kWhCost' não encontrado");
        }
    });
}
