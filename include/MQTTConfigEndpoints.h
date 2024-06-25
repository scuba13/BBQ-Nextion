#ifndef MQTT_CONFIG_ENDPOINTS_H
#define MQTT_CONFIG_ENDPOINTS_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"

void registerMQTTConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger);

#endif // MQTT_CONFIG_ENDPOINTS_H
