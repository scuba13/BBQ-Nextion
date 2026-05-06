#ifndef RESPONSE_HELPER_H
#define RESPONSE_HELPER_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"

class ResponseHelper {
public:
    static bool isAuthenticated(AsyncWebServerRequest *request, const SystemStatus& sysStat) {
        if (strlen(sysStat.apiKey) == 0) return true;
        if (!request->hasHeader("X-API-Key")) return false;
        return request->getHeader("X-API-Key")->value() == sysStat.apiKey;
    }

    static void sendUnauthorized(AsyncWebServerRequest *request) {
        sendErrorResponse(request, 401, "Chave de API inválida ou ausente");
    }

    static void sendJsonResponse(AsyncWebServerRequest *request, int statusCode, const String &message, const JsonObject &data = JsonObject()) {
        JsonDocument doc;
        doc["status"] = statusCode;
        doc["message"] = message;
        if (!data.isNull()) {
            doc["data"] = data;
        }
        String response;
        serializeJson(doc, response);

        // Criação da resposta com cabeçalhos CORS
        AsyncWebServerResponse *responseObj = request->beginResponse(statusCode, "application/json", response);
        responseObj->addHeader("Access-Control-Allow-Origin", "*");
        responseObj->addHeader("Access-Control-Allow-Methods", "PATCH, POST, GET, OPTIONS");
        responseObj->addHeader("Access-Control-Allow-Headers", "Content-Type, X-API-Key");

        // Envio da resposta
        request->send(responseObj);
    }

    static void sendErrorResponse(AsyncWebServerRequest *request, int statusCode, const String &message) {
        JsonDocument doc;
        doc["status"] = statusCode;
        doc["message"] = message;
        String response;
        serializeJson(doc, response);

        // Criação da resposta com cabeçalhos CORS
        AsyncWebServerResponse *responseObj = request->beginResponse(statusCode, "application/json", response);
        responseObj->addHeader("Access-Control-Allow-Origin", "*");
        responseObj->addHeader("Access-Control-Allow-Methods", "PATCH, POST, GET, OPTIONS");
        responseObj->addHeader("Access-Control-Allow-Headers", "Content-Type, X-API-Key");

        // Envio da resposta
        request->send(responseObj);
    }
};

#endif // RESPONSE_HELPER_H
