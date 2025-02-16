#ifndef TEMPERATURE_ENDPOINTS_H
#define TEMPERATURE_ENDPOINTS_H

#include "../Base/BaseEndpoint.h"
#include "../Base/RouteConstants.h"

class TemperatureEndpoints : public BaseEndpoint {
public:
    TemperatureEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l);
    void registerRoutes() override;
};

#endif 