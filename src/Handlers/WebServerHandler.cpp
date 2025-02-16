#include "WebServerControl.h"
#include "RouteManager.h"
#include <LittleFS.h>
#include <FS.h>

WebServerControl::WebServerControl(SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, AsyncWebServer& server)
    : _systemStatus(systemStatus)
    , _fileSystem(fileSystem)
    , _logger(logger)
    , _server(server)
{}

void WebServerControl::begin() {
    _logger.logMessage("Initializing web server...");
    
    // Criar e configurar o gerenciador de rotas
    RouteManager routeManager(_server, _systemStatus, _logger, _fileSystem);
    routeManager.registerAllRoutes();
    
    // Configurar arquivos estáticos
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    
    _server.begin();
    _logger.logMessage("Web server initialized successfully");
}

void WebServerControl::loop() {
    // Processar requisições assíncronas
    // O ESPAsyncWebServer já faz isso automaticamente
    // Este método existe apenas para manter a consistência da interface
}
