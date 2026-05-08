#include "Endpoints/DebugEndpoints.h"
#include "DebugInjector.h"
#include "Endpoints/ResponseHelper.h"
#include <ArduinoJson.h>

// Global state for debug injection (auto-expires)
DebugInjectorState debugInjector;

void registerDebugEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger) {

    // POST /api/v1/debug/inject-temp
    // Body params: bbqTemp (float), proteinTemp (float), seconds (int, default 30, max 300)
    server.on("/api/v1/debug/inject-temp", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
                ResponseHelper::sendUnauthorized(request);
                return;
            }

            float bbq = request->hasParam("bbqTemp", true)
                ? request->getParam("bbqTemp", true)->value().toFloat()
                : -1;
            float protein = request->hasParam("proteinTemp", true)
                ? request->getParam("proteinTemp", true)->value().toFloat()
                : -1;
            int seconds = request->hasParam("seconds", true)
                ? request->getParam("seconds", true)->value().toInt()
                : 30;

            if (bbq < 0 || bbq > 500 || protein < 0 || protein > 500) {
                ResponseHelper::sendErrorResponse(request, 400,
                    "bbqTemp e proteinTemp obrigatórios (0–500)");
                return;
            }
            if (seconds < 1 || seconds > 300) {
                ResponseHelper::sendErrorResponse(request, 400,
                    "seconds deve estar entre 1 e 300");
                return;
            }

            debugInjector.bbqTemp = bbq;
            debugInjector.proteinTemp = protein;
            debugInjector.endTimeMs = millis() + (unsigned long)seconds * 1000UL;
            debugInjector.active = true;

            logger.logMessage("Debug inject: bbq=" + String(bbq) +
                              " prt=" + String(protein) +
                              " por " + String(seconds) + "s");

            JsonDocument doc;
            doc["bbqTemp"] = bbq;
            doc["proteinTemp"] = protein;
            doc["seconds"] = seconds;
            ResponseHelper::sendJsonResponse(request, 200, "Injeção ativa", doc.as<JsonObject>());
        }
    );

    // DELETE /api/v1/debug/inject-temp  — cancela injeção antes do prazo
    server.on("/api/v1/debug/inject-temp", HTTP_DELETE,
        [&](AsyncWebServerRequest *request) {
            if (!ResponseHelper::isAuthenticated(request, systemStatus)) {
                ResponseHelper::sendUnauthorized(request);
                return;
            }
            debugInjector.active = false;
            logger.logMessage("Debug inject cancelado");
            ResponseHelper::sendJsonResponse(request, 200, "Injeção cancelada");
        }
    );
}
