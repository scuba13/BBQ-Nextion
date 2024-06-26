#include "MonitorEndpoints.h"
#include <ArduinoJson.h>

void registerMonitorEndpoints(AsyncWebServer &server, SystemStatus &systemStatus)
{
    server.on("/monitor", HTTP_GET, [&systemStatus](AsyncWebServerRequest *request)
              {
        String jsonResponse = "{";
        jsonResponse += "\"currentTemp\": " + String(systemStatus.calibratedTemp) + ",";
        jsonResponse += "\"setTemp\": " + String(systemStatus.bbqTemperature) + ",";
        jsonResponse += "\"proteinTemp\": " + String(systemStatus.calibratedTempP) + ",";
        jsonResponse += "\"proteinTempSet\": " + String(systemStatus.proteinTemperature) + ",";
        jsonResponse += "\"relayState\": \"" + String(systemStatus.isRelayOn ? "ON" : "OFF") + "\",";
        jsonResponse += "\"avgTemp\": " + String(systemStatus.averageTemp) + ",";
        jsonResponse += "\"caliTemp\": " + String(systemStatus.tempCalibration) + ",";
        jsonResponse += "\"minBBQTemp\": " + String(systemStatus.minBBQTemp) + ",";
        jsonResponse += "\"maxBBQTemp\": " + String(systemStatus.maxBBQTemp) + ",";
        jsonResponse += "\"minPrtTemp\": " + String(systemStatus.minPrtTemp) + ",";
        jsonResponse += "\"maxPrtTemp\": " + String(systemStatus.maxPrtTemp) + ",";
        jsonResponse += "\"minCaliTemp\": " + String(systemStatus.minCaliTemp) + ",";
        jsonResponse += "\"maxCaliTemp\": " + String(systemStatus.maxCaliTemp);
        jsonResponse += "}";
        request->send(200, "application/json", jsonResponse); });
}
