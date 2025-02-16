#ifndef AI_ENDPOINTS_H
#define AI_ENDPOINTS_H

#include "../Base/BaseEndpoint.h"
#include "../Base/RouteConstants.h"
#include "../FileSystem.h"

class AIEndpoints : public BaseEndpoint {
private:
    FileSystem& fileSystem;
public:
    AIEndpoints(AsyncWebServer& s, SystemStatus& ss, LogHandler& l, FileSystem& fs);
    void registerRoutes() override;
};

#endif
