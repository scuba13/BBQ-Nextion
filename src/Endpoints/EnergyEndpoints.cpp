#include "EnergyEndpoints.h"
#include <ArduinoJson.h>
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <Nextion.h>

void registerEnergyEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/api/v1/energy", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching energy data");

        DynamicJsonDocument doc(1024);
        doc["power"] = systemStatus.power;
        doc["energy"] = systemStatus.energy;
        doc["cost"] = systemStatus.cost;
        doc["kWhCost"] = systemStatus.kWhCost;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Energy data fetched successfully", doc.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Energy data fetched successfully");
    });

    server.on("/api/v1/energy/cost", HTTP_PATCH, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Updating energy cost");

        if (request->hasParam("kWhCost", true)) {
            float receivedNumber = request->getParam("kWhCost", true)->value().toFloat();
            systemStatus.kWhCost = receivedNumber;

            logger.logMessage("Energy cost received: " + String(systemStatus.kWhCost));

            // Utilizando ResponseHelper para enviar a resposta
            DynamicJsonDocument jsonDoc(1024);
            jsonDoc["status"] = "success";
            jsonDoc["kWhCost"] = systemStatus.kWhCost;
            ResponseHelper::sendJsonResponse(request, 200, "Energy cost updated successfully", jsonDoc.as<JsonObject>());

            logger.logMessage("Energy cost updated successfully: " + String(systemStatus.kWhCost));
        } else {
            logger.logError("Parameter 'kWhCost' not found in the request");
            ResponseHelper::sendErrorResponse(request, 400, "Parameter 'kWhCost' not found in the request");
        }
    });
}
