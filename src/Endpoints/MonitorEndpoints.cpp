#include "MonitorEndpoints.h"
#include <ArduinoJson.h>

void registerMonitorEndpoints(AsyncWebServer &server, SystemStatus &systemStatus)
{
    server.on("/monitorando", HTTP_GET, [&systemStatus](AsyncWebServerRequest *request)
    {
        StaticJsonDocument<500> doc; // Ajuste o tamanho conforme necessário
        doc["currentTemp"] = systemStatus.calibratedTemp;
        doc["setTemp"] = systemStatus.bbqTemperature;
        doc["proteinTemp"] = systemStatus.calibratedTempP;
        doc["proteinTempSet"] = systemStatus.proteinTemperature;
        doc["relayState"] = systemStatus.isRelayOn ? "ON" : "OFF";
        doc["avgTemp"] = systemStatus.averageTemp;
        doc["caliTemp"] = systemStatus.tempCalibration;
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
}
