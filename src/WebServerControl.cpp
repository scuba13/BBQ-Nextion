#include "WebServerControl.h"
#include "TemperatureEndpoints.h"
#include "MQTTConfigEndpoints.h"
#include "TempConfigEndpoints.h"
#include "EnergyEndpoints.h"
#include "GeneralEndpoints.h"
#include "AIEndpoints.h"
#include "SystemEndpoints.h"
#include "MonitorEndpoints.h"
#include <LittleFS.h>
#include <FS.h>

WebServerControl::WebServerControl(SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, AsyncWebServer& server)
    : _systemStatus(systemStatus), _fileSystem(fileSystem), _logger(logger), _server(server), _otaHandler() {}

void WebServerControl::begin() {
    // Registrar endpoints antes de configurar arquivos estáticos
    registerAIEndpoints(_server, _systemStatus, _logger, _fileSystem);
    registerSystemEndpoints(_server, _systemStatus, _logger);
    registerMonitorEndpoints(_server, _systemStatus, _logger);
    registerTemperatureEndpoints(_server, _systemStatus, _logger);
    registerMQTTConfigEndpoints(_server, _systemStatus, _fileSystem, _logger);
    registerTempConfigEndpoints(_server, _systemStatus, _fileSystem, _logger);
    registerGeneralEndpoints(_server, _systemStatus, _logger, _otaHandler);
    registerEnergyEndpoints(_server, _systemStatus, _logger);

    // Configurar arquivos estáticos
    _server.serveStatic("/static/js/", LittleFS, "/static/js/");
    _server.serveStatic("/static/css/", LittleFS, "/static/css/");
    _server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico");

    // Configuração de serveStatic deve vir depois de todas as APIs
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    _server.begin();
}

