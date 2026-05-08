#include "Endpoints/MQTTConfigEndpoints.h"
#include "Handlers/LogHandler.h"
#include "Endpoints/ResponseHelper.h"
#include "SysStatMutex.h"
#include <ArduinoJson.h>

void registerMQTTConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, MQTTHandler& mqttHandler) {
    server.on("/api/v1/mqtt/config", HTTP_GET, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        logger.logRequest(request, "Buscando configuração MQTT");

        JsonDocument doc;
        doc["mqttServer"] = systemStatus.mqttServer;
        doc["mqttPort"] = systemStatus.mqttPort;
        doc["mqttUser"] = systemStatus.mqttUser;
        doc["mqttPassword"] = "***";
        doc["isHAAvailable"] = systemStatus.isHAAvailable;

        ResponseHelper::sendJsonResponse(request, 200, "Configuração MQTT obtida com sucesso", doc.as<JsonObject>());
    });

    server.on("/api/v1/mqtt/config", HTTP_PATCH, [&systemStatus, &fileSystem, &logger, &mqttHandler](AsyncWebServerRequest *request) {
        if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
            ResponseHelper::sendUnauthorized(request);
            return;
        }
        logger.logRequest(request, "Atualizando configuração MQTT");

        String mqttServer;
        int mqttPort = 0;
        String mqttUser;
        String mqttPassword;
        bool isHAAvailable = false;

        if (request->hasParam("mqttServer", true)) {
            mqttServer = request->getParam("mqttServer", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'mqttServer' ausente");
            return;
        }

        if (request->hasParam("mqttPort", true)) {
            mqttPort = request->getParam("mqttPort", true)->value().toInt();
            if (mqttPort <= 0 || mqttPort > 65535) {
                ResponseHelper::sendErrorResponse(request, 400, "Valor de 'mqttPort' inválido");
                return;
            }
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'mqttPort' ausente");
            return;
        }

        if (request->hasParam("mqttUser", true)) {
            mqttUser = request->getParam("mqttUser", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'mqttUser' ausente");
            return;
        }

        if (request->hasParam("mqttPassword", true)) {
            mqttPassword = request->getParam("mqttPassword", true)->value();
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'mqttPassword' ausente");
            return;
        }

        if (request->hasParam("isHAAvailable", true)) {
            isHAAvailable = request->getParam("isHAAvailable", true)->value().equals("true");
        } else {
            ResponseHelper::sendErrorResponse(request, 400, "Parâmetro 'isHAAvailable' ausente");
            return;
        }

        sysStatLock();
        bool wasAvailable = systemStatus.isHAAvailable;
        strncpy(systemStatus.mqttServer, mqttServer.c_str(), sizeof(systemStatus.mqttServer) - 1);
        systemStatus.mqttServer[sizeof(systemStatus.mqttServer) - 1] = '\0';
        systemStatus.mqttPort = mqttPort;
        strncpy(systemStatus.mqttUser, mqttUser.c_str(), sizeof(systemStatus.mqttUser) - 1);
        systemStatus.mqttUser[sizeof(systemStatus.mqttUser) - 1] = '\0';
        strncpy(systemStatus.mqttPassword, mqttPassword.c_str(), sizeof(systemStatus.mqttPassword) - 1);
        systemStatus.mqttPassword[sizeof(systemStatus.mqttPassword) - 1] = '\0';
        systemStatus.isHAAvailable = isHAAvailable;
        sysStatUnlock();

        if (!fileSystem.saveConfigToFile(systemStatus)) {
            ResponseHelper::sendErrorResponse(request, 500, "Falha ao salvar configuração MQTT");
            return;
        }

        if (!wasAvailable && systemStatus.isHAAvailable) {
            mqttHandler.begin(systemStatus.mqttServer, systemStatus.mqttPort,
                              systemStatus.mqttUser, systemStatus.mqttPassword);
            startMQTTTask();
        } else if (wasAvailable && !systemStatus.isHAAvailable) {
            stopMQTTTask();
        } else if (systemStatus.isHAAvailable) {
            mqttHandler.begin(systemStatus.mqttServer, systemStatus.mqttPort,
                              systemStatus.mqttUser, systemStatus.mqttPassword);
        }

        ResponseHelper::sendJsonResponse(request, 200, "Configuração MQTT atualizada com sucesso");
    });
}
