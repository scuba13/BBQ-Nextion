#include "SystemEndpoints.h"
#include <ArduinoJson.h>

void registerSystemEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger) {
    server.on("/setTempCalibration", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        if (request->hasParam("tempCalibration", true)) {
            int tempCalibration = request->getParam("tempCalibration", true)->value().toInt();
            systemStatus.tempCalibration = tempCalibration;
            Serial.println("Temp Calibration set to: " + String(systemStatus.tempCalibration));
            logger.logMessage("Temp Calibration set to: " + String(systemStatus.tempCalibration));
            request->send(200, "application/json", "{ \"status\": \"success\" }");
        } else {
            Serial.println("Parâmetro 'tempCalibration' não encontrado na solicitação.");
        }
    });

    server.on("/activateCure", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request) {
        systemStatus.cureProcessMode = true;
        Serial.println("Cure process activated");
        logger.logMessage("Cure process activated");
        request->send(200, "application/json", "{ \"status\": \"success\" }");
    });
}