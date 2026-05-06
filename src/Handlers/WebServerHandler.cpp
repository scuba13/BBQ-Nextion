#include "WebServerControl.h"
#include "TemperatureEndpoints.h"
#include "MQTTConfigEndpoints.h"
#include "TempConfigEndpoints.h"
#include "GeneralEndpoints.h"
#include "AIEndpoints.h"
#include "SystemEndpoints.h"
#include "MonitorEndpoints.h"
#include "DiagnosticsEndpoints.h"
#include <LittleFS.h>
#include <FS.h>

WebServerControl::WebServerControl(SystemStatus& systemStatus,
                                 FileSystem& fileSystem,
                                 LogHandler& logger,
                                 AsyncWebServer& server,
                                 DiagnosticsHandler& diagnosticsHandler,
                                 MQTTHandler& mqttHandler)
    : _systemStatus(systemStatus)
    , _fileSystem(fileSystem)
    , _logger(logger)
    , _server(server)
    , _otaHandler(logger)
    , _diagnostics(diagnosticsHandler)
    , _mqttHandler(mqttHandler) {}

void WebServerControl::begin() {

    // Registrar endpoints antes de configurar arquivos estáticos
    registerAIEndpoints(_server, _systemStatus, _logger, _fileSystem);
    registerSystemEndpoints(_server, _systemStatus, _logger, _otaHandler);
    registerMonitorEndpoints(_server, _systemStatus, _logger);
    registerTemperatureEndpoints(_server, _systemStatus, _logger);
    registerMQTTConfigEndpoints(_server, _systemStatus, _fileSystem, _logger, _mqttHandler);
    registerTempConfigEndpoints(_server, _systemStatus, _fileSystem, _logger);
    registerGeneralEndpoints(_server, _systemStatus, _logger, _otaHandler);
    registerDiagnosticsEndpoints(_server, _diagnostics, _logger);

    // Adicionar handler global para OPTIONS
    _server.onNotFound([](AsyncWebServerRequest *request){
        if (request->method() == HTTP_OPTIONS) {
            AsyncWebServerResponse *response = request->beginResponse(204); // No Content
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
            response->addHeader("Access-Control-Allow-Headers", "Content-Type");
            request->send(response);
        } else {
            request->send(404, "text/plain", "Not Found");
        }
    });

    // Configurar arquivos estáticos
    _server.serveStatic("/static/js/", LittleFS, "/static/js/");
    _server.serveStatic("/static/css/", LittleFS, "/static/css/");
    _server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico");

    // Configuração de serveStatic deve vir depois de todas as APIs
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    _server.begin();
}
