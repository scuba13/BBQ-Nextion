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

        AsyncWebServerResponse *responseObj = request->beginResponse(statusCode, "application/json", response);
        responseObj->addHeader("Access-Control-Allow-Origin", "*");
        responseObj->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        responseObj->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(responseObj);
    }

    static void sendErrorResponse(AsyncWebServerRequest *request, int statusCode, const String &message) {
        DynamicJsonDocument doc(512);
        doc["status"] = statusCode;
        doc["message"] = message;
        String response;
        serializeJson(doc, response);

        AsyncWebServerResponse *responseObj = request->beginResponse(statusCode, "application/json", response);
        responseObj->addHeader("Access-Control-Allow-Origin", "*");
        responseObj->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        responseObj->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(responseObj);
    }

    static void sendCorsResponse(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Origin", "*");
        response->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type");
        request->send(response);
    }
};

#endif 