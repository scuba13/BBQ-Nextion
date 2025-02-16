#ifndef ENERGY_ENDPOINTS_H
#define ENERGY_ENDPOINTS_H

#include "../Base/BaseEndpoint.h"
#include "../Base/RouteConstants.h"

class EnergyEndpoints : public BaseEndpoint {
public:
    EnergyEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l);
    void registerRoutes() override;
};

#endif
