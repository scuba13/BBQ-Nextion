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
        request->send(statusCode, "application/json", response);
    }

    static void sendErrorResponse(AsyncWebServerRequest *request, int statusCode, const String &message) {
        DynamicJsonDocument doc(512);
        doc["status"] = statusCode;
        doc["message"] = message;
        String response;
        serializeJson(doc, response);
        request->send(statusCode, "application/json", response);
    }
};

#endif // RESPONSE_HELPER_H
