#ifndef AUTH_ENDPOINTS_H
#define AUTH_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"

void registerAuthEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger);

#endif // AUTH_ENDPOINTS_H
