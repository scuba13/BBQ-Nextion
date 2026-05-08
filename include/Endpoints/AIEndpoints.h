#ifndef AI_ENDPOINTS_H
#define AI_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/LogHandler.h"
#include "Handlers/FileSystem.h"

void registerAIEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, FileSystem& fileSystem);

#endif // AI_ENDPOINTS_H
