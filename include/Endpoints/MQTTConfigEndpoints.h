#ifndef MQTT_CONFIG_ENDPOINTS_H
#define MQTT_CONFIG_ENDPOINTS_H

#include <ESPAsyncWebServer.h>
#include "SystemStatus.h"
#include "Handlers/FileSystem.h"
#include "Handlers/LogHandler.h"
#include "Handlers/MQTTHandler.h"
#include "Handlers/TaskHandler.h"

void registerMQTTConfigEndpoints(AsyncWebServer& server, SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, MQTTHandler& mqttHandler);

#endif // MQTT_CONFIG_ENDPOINTS_H
