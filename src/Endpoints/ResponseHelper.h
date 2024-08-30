#ifndef RESPONSE_HELPER_H
#define RESPONSE_HELPER_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

class ResponseHelper {
public:
    static void sendJsonResponse(AsyncWebServerRequest *request, int statusCode, const String &message, const JsonObject &data = JsonObject()) {
        DynamicJsonDocument doc(1024);
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
        responseObj->addHeader("Access-Control-Allow-Headers", "Content-Type");

        // Envio da resposta
        request->send(responseObj);
    }

    static void sendErrorResponse(AsyncWebServerRequest *request, int statusCode, const String &message) {
        DynamicJsonDocument doc(512);
        doc["status"] = statusCode;
        doc["message"] = message;
        String response;
        serializeJson(doc, response);

        // Criação da resposta com cabeçalhos CORS
        AsyncWebServerResponse *responseObj = request->beginResponse(statusCode, "application/json", response);
        responseObj->addHeader("Access-Control-Allow-Origin", "*");
        responseObj->addHeader("Access-Control-Allow-Methods", "PATCH, POST, GET, OPTIONS");
        responseObj->addHeader("Access-Control-Allow-Headers", "Content-Type");

        // Envio da resposta
        request->send(responseObj);
    }
};

#endif // RESPONSE_HELPER_H
