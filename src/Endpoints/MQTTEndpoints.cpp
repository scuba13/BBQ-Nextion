#include "MQTTEndpoints.h"
#include <ArduinoJson.h>

MQTTEndpoints::MQTTEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l, FileSystem& fs)
    : BaseEndpoint(s, ss, l), fileSystem(fs) {}

void MQTTEndpoints::registerRoutes() {
    // GET config
    server.on(Routes::MQTT::CONFIG, HTTP_GET, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::MQTT::CONFIG, "GET");

        DynamicJsonDocument doc(1024);
        doc["mqttServer"] = systemStatus.mqttServer;
        doc["mqttPort"] = systemStatus.mqttPort;
        doc["mqttUser"] = systemStatus.mqttUser;
        doc["mqttPassword"] = systemStatus.mqttPassword;
        doc["isHAAvailable"] = systemStatus.isHAAvailable;

        ResponseHelper::sendJsonResponse(request, 200, "MQTT configuration fetched", doc.as<JsonObject>());
    });

    // PATCH config
    server.on(Routes::MQTT::CONFIG, HTTP_PATCH, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::MQTT::CONFIG, "PATCH");

        if (!validateParam(request, "mqttServer") || 
            !validateParam(request, "mqttPort") ||
            !validateParam(request, "mqttUser") ||
            !validateParam(request, "mqttPassword") ||
            !validateParam(request, "isHAAvailable")) {
            return;
        }

        String mqttServer = request->getParam("mqttServer", true)->value();
        int mqttPort = request->getParam("mqttPort", true)->value().toInt();
        String mqttUser = request->getParam("mqttUser", true)->value();
        String mqttPassword = request->getParam("mqttPassword", true)->value();
        bool isHAAvailable = request->getParam("isHAAvailable", true)->value() == "true";

        if (mqttPort <= 0 || mqttPort > 65535) {
            ResponseHelper::sendErrorResponse(request, 400, "Invalid port number");
            return;
        }

        strncpy(systemStatus.mqttServer, mqttServer.c_str(), sizeof(systemStatus.mqttServer) - 1);
        systemStatus.mqttPort = mqttPort;
        strncpy(systemStatus.mqttUser, mqttUser.c_str(), sizeof(systemStatus.mqttUser) - 1);
        strncpy(systemStatus.mqttPassword, mqttPassword.c_str(), sizeof(systemStatus.mqttPassword) - 1);
        systemStatus.isHAAvailable = isHAAvailable;

        fileSystem.saveConfigToFile(systemStatus);
        ResponseHelper::sendJsonResponse(request, 200, "MQTT configuration updated");
    });
} 