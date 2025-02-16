#ifndef MONITOR_ENDPOINTS_H
#define MONITOR_ENDPOINTS_H

#include "../Base/BaseEndpoint.h"
#include "../Base/RouteConstants.h"

class MonitorEndpoints : public BaseEndpoint {
public:
    MonitorEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l);
    void registerRoutes() override;
};

#endif
