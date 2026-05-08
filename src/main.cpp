#include <WiFiManager.h>
#include "SystemStatus.h"
#include "SysStatMutex.h"
#include "PinDefinitions.h"
#include "Handlers/WebServerHandler.h"
#include "Handlers/TemperatureHandler.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include "Handlers/MQTTHandler.h"
#include <ESPmDNS.h>
#include "Handlers/LogHandler.h"
#include "Handlers/FileSystem.h"
#include "Handlers/NextionHandler.h"
#include "Handlers/TaskHandler.h"
#include "Handlers/WiFiHandler.h"
#include "Handlers/DiagnosticsHandler.h"

SemaphoreHandle_t sysStatMutex = nullptr;

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
    // Relé PRIMEIRO: evita estado indefinido no GPIO durante os milissegundos do boot
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    Serial.begin(115200);
    neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
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

    // Registra causa do último reboot — essencial para debugging em campo
    static const char* resetReasons[] = {
        "desconhecido", "power-on", "reset externo", "software",
        "panic/exception", "watchdog int.", "watchdog task",
        "watchdog outros", "sleep profundo", "brownout", "SDIO"
    };
    int rrIdx = (int)esp_reset_reason();
    if (rrIdx < 0 || rrIdx > 10) rrIdx = 0;
    logHandler.logMessage("Causa do reboot: " + String(resetReasons[rrIdx]));

    // Configura MQTT se habilitado
    if (sysStat.isHAAvailable) {
        mqttHandler.begin(sysStat.mqttServer, sysStat.mqttPort,
                          sysStat.mqttUser, sysStat.mqttPassword);
    }

    // Mutex recursivo para proteger sysStat entre tasks e cores
    sysStatMutex = xSemaphoreCreateRecursiveMutex();

    // Inicia tasks de temperatura, controle e MQTT
    initializeTasks(sysStat, mqttHandler);

    // Inicia task de diagnóstico (60s)
    startDiagnosticsTask(diagnostics);

    // Inicia servidor web
    webServerControl.begin();

    diagnostics.logMetrics();
}

void loop() {
    // nexLoop processa callbacks Nextion (que escrevem em sysStat sob seu próprio lock)
    nexLoop(nex_listen_list);

    // getCurrentPageId usa Serial2 apenas — sem acesso a sysStat, fora do lock
    uint8_t currentPage = getCurrentPageId();

    // Cada função de update copia os campos necessários do sysStat sob seu próprio
    // lock interno e faz as escritas seriais fora do mutex (B-03)
    updateNextionMonitorVariables(sysStat, currentPage);
    updateNextionSetBBQVariables(sysStat, currentPage);
    updateNextionSetChunkVariables(sysStat, currentPage);
    updateNextionSetCaliVariables(sysStat, currentPage);

    diagnostics.watchdogFeed();

    if (!diagnostics.isHealthy()) {
        logHandler.logError("Sistema com recursos críticos!");
    }

    // Lê isRelayOn sob lock para atualizar LED (único ponto de neopixelWrite)
    sysStatLock();
    bool relayOn = sysStat.isRelayOn;
    sysStatUnlock();

    static bool lastRelayState = false;
    if (relayOn != lastRelayState) {
        lastRelayState = relayOn;
        if (relayOn) {
            neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red = relay ON
        } else {
            neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue = relay OFF
        }
    }

    vTaskDelay(1);
}
