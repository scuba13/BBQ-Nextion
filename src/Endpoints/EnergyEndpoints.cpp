#include "EnergyEndpoints.h"
#include <ArduinoJson.h>
#include <Nextion.h>


void registerEnergyEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/getEnergy", HTTP_GET, [&systemStatus](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["power"] = systemStatus.power;
        doc["energy"] = systemStatus.energy;
        doc["cost"] = systemStatus.cost;
        doc["kWhCost"] = systemStatus.kWhCost;

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/setEnergyCost", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        dbSerial.println("Requisição recebida Set EnergyCost");
        logger.logMessage("Requisição recebida Set EnergyCost");

        if (request->hasParam("kWhCost", true)) {
            float receivedNumber = request->getParam("kWhCost", true)->value().toFloat();
            systemStatus.kWhCost = receivedNumber;
            dbSerial.println("EnergyCost recebida: " + String(systemStatus.kWhCost));
            logger.logMessage("EnergyCost recebida: " + String(systemStatus.kWhCost));

            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<200> jsonDoc;
            jsonDoc["status"] = "success";
            jsonDoc["kWhCost"] = systemStatus.kWhCost;
            serializeJson(jsonDoc, *response);
            request->send(response);
            dbSerial.println("EnergyCost definida: " + String(systemStatus.kWhCost));
            logger.logMessage("EnergyCost definida: " + String(systemStatus.kWhCost));

        } else {
            dbSerial.println("Parâmetro 'kWhCost' não encontrado na solicitação.");
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<200> jsonDoc;
            jsonDoc["status"] = "error";
            jsonDoc["message"] = "Parameter 'kWhCost' not found in the request.";
            serializeJson(jsonDoc, *response);
            request->send(response);
            dbSerial.println("Parâmetro 'kWhCost' não encontrado na solicitação.");
            logger.logMessage("Parâmetro 'kWhCost' não encontrado na solicitação.");
        }
    });
}
