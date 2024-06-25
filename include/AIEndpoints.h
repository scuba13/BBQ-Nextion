#ifndef AI_ENDPOINTS_H
#define AI_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "FileSystem.h"

void registerAIEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, LogHandler& logger, FileSystem& fileSystem);

#endif // AI_ENDPOINTS_H
