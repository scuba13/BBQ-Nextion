#include "SystemEndpoints.h"
#include "LogHandler.h"     // Inclua o novo LogHandler aqui
#include "ResponseHelper.h" // Inclua o ResponseHelper aqui
#include <ArduinoJson.h>
#include <Nextion.h>
#include "OTAHandler.h" 
#include "TemperatureControl.h"

OTAHandler otaHandler; // Instância do OTAHandler

void registerSystemEndpoints(AsyncWebServer &server, SystemStatus &systemStatus, LogHandler &logger)
{
    server.on("/api/v1/system/activateCure", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        systemStatus.cureProcessMode = true;

        logger.logRequest(request, "Cure process activated");

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "Cure process activated successfully");

        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("Cure process activated successfully"); });

    // Novo endpoint para resetar o sistema
    server.on("/api/v1/system/reset", HTTP_POST, [&systemStatus, &logger](AsyncWebServerRequest *request)
              {
        // Chama o método resetSystem passando o systemStatus como parâmetro
        resetSystem(systemStatus);

        logger.logRequest(request, "System reset initiated");

        // Utilizando ResponseHelper para enviar a resposta
        ResponseHelper::sendJsonResponse(request, 200, "System reset successfully");

        // Log da mensagem de sucesso utilizando o novo LogHandler
        logger.logMessage("System reset successfully"); });

    // Novo endpoint para atualização de firmware OTA
    server.on(
        "/api/v1/system/updateFirmware", HTTP_POST, [](AsyncWebServerRequest *request) {},
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            otaHandler.handleFirmwareUpload(request, filename, index, data, len, final);
        });
}
