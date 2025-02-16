#ifndef MQTT_ENDPOINTS_H
#define MQTT_ENDPOINTS_H

#include "../Base/BaseEndpoint.h"
#include "../Base/RouteConstants.h"
#include "../FileSystem.h"

class MQTTEndpoints : public BaseEndpoint {
private:
    FileSystem& fileSystem;

public:
    MQTTEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l, FileSystem& fs);
    void registerRoutes() override;
};

#endif 