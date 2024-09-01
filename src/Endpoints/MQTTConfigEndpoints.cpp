#include "MQTTConfigEndpoints.h"
#include "LogHandler.h"       // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"   // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>
#include <Nextion.h>

void registerMQTTConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger) {
    server.on("/api/v1/mqtt/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        // Log da requisição utilizando o novo LogHandler
        logger.logRequest(request, "Fetching MQTT configuration");

        DynamicJsonDocument doc(1024);
        doc["mqttServer"] = systemStatus.mqttServer;
        doc["mqttPort"] = systemStatus.mqttPort;
        doc["mqttUser"] = systemStatus.mqttUser;
        doc["mqttPassword"] = systemStatus.mqttPassword;
        doc["isHAAvailable"] = systemStatus.isHAAvailable;

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "MQTT configuration fetched successfully", doc.as<JsonObject>());
        
        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("MQTT configuration fetched successfully");
    });

    server.on("/api/v1/mqtt/config", HTTP_PATCH, [&systemStatus, &fileSystem, &logger](AsyncWebServerRequest *request) {
    // Log da requisição utilizando o novo LogHandler
    logger.logRequest(request, "Updating MQTT configuration");

    String mqttServer;
    int mqttPort = 0; // Inicializado com um valor padrão
    String mqttUser;
    String mqttPassword;
    bool isHAAvailable = false;

    if (request->hasParam("mqttServer", true)) {
        mqttServer = request->getParam("mqttServer", true)->value();
    } else {
        logger.logError("mqttServer parameter is missing");
        ResponseHelper::sendErrorResponse(request, 400, "mqttServer parameter is missing");
        return;
    }

    if (request->hasParam("mqttPort", true)) {
        mqttPort = request->getParam("mqttPort", true)->value().toInt();
        if (mqttPort <= 0 || mqttPort > 65535) {
            logger.logError("Invalid mqttPort value");
            ResponseHelper::sendErrorResponse(request, 400, "Invalid mqttPort value");
            return;
        }
    } else {
        logger.logError("mqttPort parameter is missing");
        ResponseHelper::sendErrorResponse(request, 400, "mqttPort parameter is missing");
        return;
    }

    if (request->hasParam("mqttUser", true)) {
        mqttUser = request->getParam("mqttUser", true)->value();
    } else {
        logger.logError("mqttUser parameter is missing");
        ResponseHelper::sendErrorResponse(request, 400, "mqttUser parameter is missing");
        return;
    }

    if (request->hasParam("mqttPassword", true)) {
        mqttPassword = request->getParam("mqttPassword", true)->value();
    } else {
        logger.logError("mqttPassword parameter is missing");
        ResponseHelper::sendErrorResponse(request, 400, "mqttPassword parameter is missing");
        return;
    }

    if (request->hasParam("isHAAvailable", true)) {
        isHAAvailable = request->getParam("isHAAvailable", true)->value().equals("true");
    } else {
        logger.logError("isHAAvailable parameter is missing");
        ResponseHelper::sendErrorResponse(request, 400, "isHAAvailable parameter is missing");
        return;
    }

    // Salva as configurações no banco de dados e atualiza o SystemStatus
    fileSystem.saveConfigToDatabase("mqttServer", mqttServer.c_str(), systemStatus);
    fileSystem.saveConfigToDatabase("mqttPort", String(mqttPort).c_str(), systemStatus);
    fileSystem.saveConfigToDatabase("mqttUser", mqttUser.c_str(), systemStatus);
    fileSystem.saveConfigToDatabase("mqttPassword", mqttPassword.c_str(), systemStatus);
    fileSystem.saveConfigToDatabase("isHAAvailable", isHAAvailable ? "true" : "false", systemStatus);

    // Utilizando ResponseHelper para enviar a resposta
    ResponseHelper::sendJsonResponse(request, 200, "MQTT configuration updated successfully");

    // Log da mensagem de sucesso utilizando o novo LogHandler
    logger.logMessage("MQTT configuration updated successfully");
});

}
