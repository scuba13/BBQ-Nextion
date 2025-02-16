#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <vector>
#include <memory>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "FileSystem.h"

// Includes dos endpoints
#include "Endpoints/TemperatureEndpoints.h"
#include "Endpoints/SystemEndpoints.h"
#include "Endpoints/MonitorEndpoints.h"
#include "Endpoints/MQTTEndpoints.h"
#include "Endpoints/EnergyEndpoints.h"
#include "Endpoints/AIEndpoints.h"

class RouteManager {
private:
    AsyncWebServer& server;
    SystemStatus& systemStatus;
    LogHandler& logger;
    FileSystem& fileSystem;
    std::vector<std::unique_ptr<BaseEndpoint>> endpoints;

    void handleNotFound(AsyncWebServerRequest *request);
    void setupCORS();

public:
    RouteManager(AsyncWebServer& s, SystemStatus& ss, LogHandler& l, FileSystem& fs);
    ~RouteManager() = default; // O std::unique_ptr limpará automaticamente
    void registerAllRoutes();
};

#endif 