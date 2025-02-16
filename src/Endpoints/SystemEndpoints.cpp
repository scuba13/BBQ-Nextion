#include "SystemEndpoints.h"
#include "LogHandler.h"     // Inclua o novo LogHandler aqui
#include "ResponseHelper.h" // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>
#include <Nextion.h>
#include "OTAHandler.h" 
#include "TemperatureControl.h"

SystemEndpoints::SystemEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l)
    : BaseEndpoint(s, ss, l)
    , _otaHandler(l)  // Inicializa o OTAHandler com o logger
{}

void SystemEndpoints::registerRoutes() {
    server.on(Routes::System::RESET, HTTP_POST, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::System::RESET, "POST");
        // Implementação do reset
        ResponseHelper::sendJsonResponse(request, 200, "System reset completed");
    });

    server.on(Routes::System::CURE, HTTP_POST, [this](AsyncWebServerRequest *request) {
        logEndpointAccess(Routes::System::CURE, "POST");
        systemStatus.cureProcessMode = true;
        ResponseHelper::sendJsonResponse(request, 200, "Cure process activated");
    });

    // Endpoint para atualização de firmware OTA
    server.on(
        Routes::System::UPDATE,
        HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            _otaHandler.handleFirmwareUpload(request, filename, index, data, len, final);
        }
    );
}
