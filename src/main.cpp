#include <WiFiManager.h>
#include "SystemStatus.h"
#include "PinDefinitions.h"
#include "WebServerControl.h"
#include "TemperatureControl.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include "MQTTHandler.h"
#include <ESPmDNS.h>
#include "LogHandler.h"
#include "FileSystem.h"
#include "NextionHandler.h"
#include "TaskHandler.h"
#include "WiFiHandler.h"
#include "DiagnosticsHandler.h"

// Instanciação dos objetos globais
SystemStatus sysStat;
FileSystem fileSystem;
WiFiClient net;
PubSubClient client(net);
LogHandler logHandler;
LogHandler _logger;
AsyncWebServer server(80);
DiagnosticsHandler diagnostics(logHandler);
WebServerControl webServerControl(sysStat, 
                                fileSystem, 
                                logHandler, 
                                server,
                                diagnostics);
MQTTHandler mqttHandler(net, client, sysStat, logHandler);

// Inicialização rápida
void fastInit() {
    neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    initNextion(sysStat);
    initial.show();
    WiFi.begin();
}

void setup() {
    fastInit();
    
    // Inicializa tasks de MQTT e controle
    initializeTasks(sysStat, mqttHandler);
    
    // Inicializa tasks de temperatura
    createTasks();
    
    // Inicializa serviços
    webServerControl.begin();
    fileSystem.initializeAndLoadConfig(sysStat, WiFi.macAddress());
    
    // Inicializa diagnósticos
    diagnostics.logMetrics(); // Log inicial
}

void loop() {
    nexLoop(nex_listen_list);
    
    // Atualiza todas as variáveis do Nextion
    updateNextionMonitorVariables(sysStat);
    updateNextionSetBBQVariables(sysStat);
    updateNextionSetChunkVariables(sysStat);
    updateNextionSetCaliVariables(sysStat);
    
    // Watchdog e monitoramento de tasks
    diagnostics.watchdogFeed();
    diagnostics.checkTasks();
    diagnostics.logMetrics();
    
    if (!diagnostics.isHealthy()) {
        _logger.logError("Sistema com recursos críticos!");
    }
    
    vTaskDelay(1);
}
