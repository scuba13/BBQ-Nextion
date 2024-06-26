#include "TempConfigEndpoints.h"
#include <ArduinoJson.h>


void registerTempConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger) {
    server.on("/getTempConfig", HTTP_GET, [&systemStatus](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["minBBQTemp"] = systemStatus.minBBQTemp;
        doc["maxBBQTemp"] = systemStatus.maxBBQTemp;
        doc["minPrtTemp"] = systemStatus.minPrtTemp;
        doc["maxPrtTemp"] = systemStatus.maxPrtTemp;
        doc["minCaliTemp"] = systemStatus.minCaliTemp;
        doc["maxCaliTemp"] = systemStatus.maxCaliTemp;
        doc["minCaliTempP"] = systemStatus.minCaliTempP;
        doc["maxCaliTempP"] = systemStatus.maxCaliTempP;

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        request->send(200, "application/json", jsonResponse);
    });

    server.on("/updateTempConfig", HTTP_POST, [&systemStatus, &fileSystem, &logger](AsyncWebServerRequest *request) {
        logger.logMessage("Updating TempConfig configuration");
        Serial.println("Updating TempConfig configuration");

        int newMinBBQTemp, newMaxBBQTemp, newMinPrtTemp, newMaxPrtTemp, newMinCaliTemp, newMaxCaliTemp, newMinCaliTempP, newMaxCaliTempP;
        if (request->hasParam("minBBQTemp", true)) {
            newMinBBQTemp = request->getParam("minBBQTemp", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"minBBQTemp parameter is missing\" }");
            return;
        }
        if (request->hasParam("maxBBQTemp", true)) {
            newMaxBBQTemp = request->getParam("maxBBQTemp", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"maxBBQTemp parameter is missing\" }");
            return;
        }
        if (request->hasParam("minPrtTemp", true)) {
            newMinPrtTemp = request->getParam("minPrtTemp", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"minPrtTemp parameter is missing\" }");
            return;
        }
        if (request->hasParam("maxPrtTemp", true)) {
            newMaxPrtTemp = request->getParam("maxPrtTemp", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"maxPrtTemp parameter is missing\" }");
            return;
        }
        if (request->hasParam("minCaliTemp", true)) {
            newMinCaliTemp = request->getParam("minCaliTemp", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"minCaliTemp parameter is missing\" }");
            return;
        }
        if (request->hasParam("maxCaliTemp", true)) {
            newMaxCaliTemp = request->getParam("maxCaliTemp", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"maxCaliTemp parameter is missing\" }");
            return;
        }
        if (request->hasParam("minCaliTempP", true)) {
            newMinCaliTempP = request->getParam("minCaliTempP", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"minCaliTempP parameter is missing\" }");
            return;
        }
        if (request->hasParam("maxCaliTempP", true)) {
            newMaxCaliTempP = request->getParam("maxCaliTempP", true)->value().toInt();
        } else {
            request->send(400, "application/json", "{ \"error\": \"maxCaliTempP parameter is missing\" }");
            return;
        }
        

        systemStatus.minBBQTemp = newMinBBQTemp;
        systemStatus.maxBBQTemp = newMaxBBQTemp;
        systemStatus.minPrtTemp = newMinPrtTemp;
        systemStatus.maxPrtTemp = newMaxPrtTemp;
        systemStatus.minCaliTemp = newMinCaliTemp;
        systemStatus.maxCaliTemp = newMaxCaliTemp;
        systemStatus.minCaliTempP = newMinCaliTempP;
        systemStatus.maxCaliTempP = newMaxCaliTempP;

        fileSystem.saveConfigToFile(systemStatus);

        logger.logMessage("TempConfig configuration updated");
        Serial.println("TempConfig configuration updated");
        request->send(200, "application/json", "{ \"status\": \"success\" }");
    });
}
