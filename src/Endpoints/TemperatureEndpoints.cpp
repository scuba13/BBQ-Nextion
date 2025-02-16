#include "TemperatureEndpoints.h"
#include "LogHandler.h"      // Inclua o novo LogHandler aqui
#include "ResponseHelper.h"  // Inclua o ResponseHelper aqui
#include "BaseEndpoint.h"
#include "RouteConstants.h"
#include <ArduinoJson.h>

TemperatureEndpoints::TemperatureEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l)
    : BaseEndpoint(s, ss, l) {}

void TemperatureEndpoints::registerRoutes() {
    // GET config
    server.on(Routes::Temperature::CONFIG, HTTP_GET, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::Temperature::CONFIG, "GET");

        DynamicJsonDocument doc(1024);
        doc["bbqTemperature"] = systemStatus.bbqTemperature;
        doc["proteinTemperature"] = systemStatus.proteinTemperature;
        doc["tempCalibration"] = systemStatus.tempCalibration;
        doc["tempCalibrationP"] = systemStatus.tempCalibrationP;
        doc["minBBQTemp"] = systemStatus.minBBQTemp;
        doc["maxBBQTemp"] = systemStatus.maxBBQTemp;
        doc["minPrtTemp"] = systemStatus.minPrtTemp;
        doc["maxPrtTemp"] = systemStatus.maxPrtTemp;
        doc["minCaliTemp"] = systemStatus.minCaliTemp;
        doc["maxCaliTemp"] = systemStatus.maxCaliTemp;
        doc["minCaliTempP"] = systemStatus.minCaliTempP;
        doc["maxCaliTempP"] = systemStatus.maxCaliTempP;

        ResponseHelper::sendJsonResponse(request, 200, "Temperature config fetched", doc.as<JsonObject>());
    });

    // PATCH config
    server.on(Routes::Temperature::CONFIG, HTTP_PATCH, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::Temperature::CONFIG, "PATCH");

        bool updated = false;
        
        if (validateParam(request, "bbqTemperature", false)) {
            systemStatus.bbqTemperature = request->getParam("bbqTemperature", true)->value().toFloat();
            updated = true;
        }
        
        if (validateParam(request, "proteinTemperature", false)) {
            systemStatus.proteinTemperature = request->getParam("proteinTemperature", true)->value().toFloat();
            updated = true;
        }
        
        if (validateParam(request, "tempCalibration", false)) {
            systemStatus.tempCalibration = request->getParam("tempCalibration", true)->value().toFloat();
            updated = true;
        }
        
        if (validateParam(request, "tempCalibrationP", false)) {
            systemStatus.tempCalibrationP = request->getParam("tempCalibrationP", true)->value().toFloat();
            updated = true;
        }

        if (!updated) {
            ResponseHelper::sendErrorResponse(request, 400, "No valid parameters provided");
            return;
        }

        ResponseHelper::sendJsonResponse(request, 200, "Temperature config updated");
    });
}
