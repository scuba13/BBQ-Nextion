#include "TemperatureEndpoints.h"
#include "TemperatureControl.h"
#include <ArduinoJson.h>
#include <Nextion.h>



void registerTemperatureEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/setTemp", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        dbSerial.println("Requisição recebida Set BBQ Temperature");
        if (request->hasParam("temp", true)) {
            int receivedNumber = request->getParam("temp", true)->value().toInt();
            systemStatus.bbqTemperature = receivedNumber;
            dbSerial.println("Temperatura definida recebida: " + String(systemStatus.bbqTemperature));
            logger.logMessage("Temperatura definida recebida: " + String(systemStatus.bbqTemperature));

            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<200> jsonDoc;
            jsonDoc["status"] = "success";
            jsonDoc["temp"] = systemStatus.bbqTemperature;
            serializeJson(jsonDoc, *response);
            request->send(response);
        } else {
            dbSerial.println("Parâmetro 'temp' não encontrado na solicitação.");
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<200> jsonDoc;
            jsonDoc["status"] = "error";
            jsonDoc["message"] = "Parameter 'temp' not found in the request.";
            serializeJson(jsonDoc, *response);
            request->send(response);
        }
    });

    server.on("/setProteinTemp", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        dbSerial.println("Requisição recebida Set Protein Temperature");
        if (request->hasParam("proteinTemp", true)) {
            int receivedNumber = request->getParam("proteinTemp", true)->value().toInt();
            systemStatus.proteinTemperature = receivedNumber;
            dbSerial.println("Protein temperature set to: " + String(systemStatus.proteinTemperature));
            logger.logMessage("Protein temperature set to: " + String(systemStatus.proteinTemperature));

            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<200> jsonDoc;
            jsonDoc["status"] = "success";
            jsonDoc["proteinTemp"] = systemStatus.proteinTemperature;
            serializeJson(jsonDoc, *response);
            request->send(response);
        } else {
            dbSerial.println("Parâmetro 'proteinTemp' não encontrado na solicitação.");
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<200> jsonDoc;
            jsonDoc["status"] = "error";
            jsonDoc["message"] = "Parameter 'proteinTemp' not found in the request.";
            serializeJson(jsonDoc, *response);
            request->send(response);
        }
    });
}
