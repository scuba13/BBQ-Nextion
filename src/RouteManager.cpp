#include "RouteManager.h"

RouteManager::RouteManager(AsyncWebServer& s, SystemStatus& ss, LogHandler& l, FileSystem& fs)
    : server(s), systemStatus(ss), logger(l), fileSystem(fs) {
    
    // Inicializar endpoints usando new (vamos limpar no destrutor)
    endpoints.push_back(std::unique_ptr<BaseEndpoint>(new TemperatureEndpoints(server, systemStatus, logger)));
    endpoints.push_back(std::unique_ptr<BaseEndpoint>(new SystemEndpoints(server, systemStatus, logger)));
    endpoints.push_back(std::unique_ptr<BaseEndpoint>(new MonitorEndpoints(server, systemStatus, logger)));
    endpoints.push_back(std::unique_ptr<BaseEndpoint>(new MQTTEndpoints(server, systemStatus, logger, fileSystem)));
    endpoints.push_back(std::unique_ptr<BaseEndpoint>(new EnergyEndpoints(server, systemStatus, logger)));
    endpoints.push_back(std::unique_ptr<BaseEndpoint>(new AIEndpoints(server, systemStatus, logger, fileSystem)));
}

void RouteManager::setupCORS() {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void RouteManager::handleNotFound(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
        request->send(204);
    } else {
        request->send(404, "text/plain", "Not Found");
    }
}

void RouteManager::registerAllRoutes() {
    setupCORS();
    
    // Registrar todas as rotas
    for (auto& endpoint : endpoints) {
        endpoint->registerRoutes();
    }
    
    // Handler global para rotas não encontradas
    server.onNotFound([this](AsyncWebServerRequest *request) {
        handleNotFound(request);
    });
    
    logger.logMessage("All routes registered successfully");
} 