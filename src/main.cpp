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
AsyncWebServer server(80);
DiagnosticsHandler diagnostics(logHandler);
MQTTHandler mqttHandler(net, client, sysStat, logHandler);
WebServerControl webServerControl(sysStat,
                                fileSystem,
                                logHandler,
                                server,
                                diagnostics,
                                mqttHandler);

// Inicialização rápida de hardware (sem WiFi)
void fastInit() {
    Serial.begin(115200);
    neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    initNextion(sysStat);
    initial.show();
}

void setup() {
    fastInit();

    // WiFi — tenta reconectar com credenciais salvas, abre portal AP se necessário
    initWiFi(sysStat, logHandler);

    // Carrega configurações persistidas
    fileSystem.initializeAndLoadConfig(sysStat, WiFi.macAddress());

    // Inicializa log após LittleFS estar disponível
    logHandler.begin();

    // Configura MQTT se habilitado
    if (sysStat.isHAAvailable) {
        mqttHandler.begin(sysStat.mqttServer, sysStat.mqttPort,
                          sysStat.mqttUser, sysStat.mqttPassword);
    }

    // Inicia tasks de temperatura, controle e MQTT
    initializeTasks(sysStat, mqttHandler);

    // Inicia servidor web
    webServerControl.begin();

    diagnostics.logMetrics();
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
        logHandler.logError("Sistema com recursos críticos!");
    }
    
    vTaskDelay(1);
}
