#include "MQTTConfigEndpoints.h"
#include <ArduinoJson.h>



void registerMQTTConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger) {
    server.on("/getMQTTConfig", HTTP_GET, [&systemStatus](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["mqttServer"] = systemStatus.mqttServer;
        doc["mqttPort"] = systemStatus.mqttPort;
        doc["mqttUser"] = systemStatus.mqttUser;
        doc["mqttPassword"] = systemStatus.mqttPassword;
        doc["isHAAvailable"] = systemStatus.isHAAvailable;

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/updateMQTTConfig", HTTP_POST, [&systemStatus, &fileSystem, &logger](AsyncWebServerRequest *request) {
        logger.logMessage("Updating MQTT configuration");
        Serial.println("Updating MQTT configuration");

        String mqttServer;
        int mqttPort = 0; // Inicializado com um valor padrão
        String mqttUser;
        String mqttPassword;
        bool isHAAvailable = false;

        if (request->hasParam("mqttServer", true)) {
            mqttServer = request->getParam("mqttServer", true)->value();
        } else {
            request->send(400, "application/json", "{ \"error\": \"mqttServer parameter is missing\" }");
            return;
        }

        if (request->hasParam("mqttPort", true)) {
            mqttPort = request->getParam("mqttPort", true)->value().toInt();
            if (mqttPort <= 0 || mqttPort > 65535) {
                request->send(400, "application/json", "{ \"error\": \"Invalid mqttPort value\" }");
                return;
            }
        } else {
            request->send(400, "application/json", "{ \"error\": \"mqttPort parameter is missing\" }");
            return;
        }

        if (request->hasParam("mqttUser", true)) {
            mqttUser = request->getParam("mqttUser", true)->value();
        } else {
            request->send(400, "application/json", "{ \"error\": \"mqttUser parameter is missing\" }");
            return;
        }

        if (request->hasParam("mqttPassword", true)) {
            mqttPassword = request->getParam("mqttPassword", true)->value();
        } else {
            request->send(400, "application/json", "{ \"error\": \"mqttPassword parameter is missing\" }");
            return;
        }

        if (request->hasParam("isHAAvailable", true)) {
            isHAAvailable = request->getParam("isHAAvailable", true)->value().equals("true");
        } else {
            request->send(400, "application/json", "{ \"error\": \"isHAAvailable parameter is missing\" }");
            return;
        }

        strncpy(systemStatus.mqttServer, mqttServer.c_str(), sizeof(systemStatus.mqttServer) - 1);
        systemStatus.mqttServer[sizeof(systemStatus.mqttServer) - 1] = '\0';
        systemStatus.mqttPort = mqttPort;
        strncpy(systemStatus.mqttUser, mqttUser.c_str(), sizeof(systemStatus.mqttUser) - 1);
        systemStatus.mqttUser[sizeof(systemStatus.mqttUser) - 1] = '\0';
        strncpy(systemStatus.mqttPassword, mqttPassword.c_str(), sizeof(systemStatus.mqttPassword) - 1);
        systemStatus.mqttPassword[sizeof(systemStatus.mqttPassword) - 1] = '\0';
        systemStatus.isHAAvailable = isHAAvailable;

        fileSystem.saveConfigToFile(systemStatus);

        logger.logMessage("MQTT configuration updated");
        Serial.println("MQTT configuration updated");
        request->send(200, "application/json", "{ \"status\": \"success\" }");
    });
}
