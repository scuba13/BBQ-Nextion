#ifndef WIFIHANDLER_H
#define WIFIHANDLER_H

#include <WiFiManager.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include <ESPmDNS.h>

// Declaração das funções
void initWiFi(SystemStatus &sysStat, LogHandler &logHandler);

#endif // WIFIHANDLER_H
