#ifndef SYSTEM_ENDPOINTS_H
#define SYSTEM_ENDPOINTS_H

#include "../Base/BaseEndpoint.h"
#include "../Base/RouteConstants.h"
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "FileSystem.h"
#include "OTAHandler.h"

class SystemEndpoints : public BaseEndpoint {
private:
    OTAHandler _otaHandler;  // Membro da classe

public:
    SystemEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l);
    void registerRoutes() override;
};

void registerSystemEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger);

#endif // SYSTEM_ENDPOINTS_H
