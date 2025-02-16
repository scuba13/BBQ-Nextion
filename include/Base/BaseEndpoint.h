#ifndef BASE_ENDPOINT_H
#define BASE_ENDPOINT_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "ResponseHelper.h"
#include "RouteConstants.h"

class BaseEndpoint {
protected:
    AsyncWebServer& server;
    SystemStatus& systemStatus;
    LogHandler& logger;

    bool validateParam(AsyncWebServerRequest* request, const char* paramName, bool isRequired = true) {
        if (!request->hasParam(paramName, true)) {
            if (isRequired) {
                ResponseHelper::sendErrorResponse(request, 400, String(paramName) + " parameter is missing");
                logger.logError(String(paramName) + " parameter is missing");
            }
            return false;
        }
        return true;
    }

    void logEndpointAccess(const char* endpoint, const char* method) {
        logger.logMessage(String(method) + " " + endpoint);
    }

public:
    BaseEndpoint(AsyncWebServer& s, SystemStatus& ss, LogHandler& l) 
        : server(s), systemStatus(ss), logger(l) {}

    virtual void registerRoutes() = 0;
};

#endif 